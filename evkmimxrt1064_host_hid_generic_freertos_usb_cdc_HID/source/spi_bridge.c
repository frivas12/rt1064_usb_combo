/*
 * SPI bridge task
 *
 * Streams a compact register map over the LPSPI1 slave interface so the
 * companion V70 can mirror HID traffic. The task builds a contiguous memory
 * image (hub status + per-device IN/OUT blocks), pushes it over SPI, and
 * ingests any host-side writes to keep the shared state in sync.
 */
#include "spi_bridge.h"

#include "FreeRTOS.h"
#include "app.h"
#include "board.h"
#include "fsl_clock.h"
#include "pin_mux.h"

#include <stdio.h>
#include <string.h>

#define SPI_BRIDGE_SPI_TIMEOUT (1000000U)

#ifndef SPI_BRIDGE_SPI_BASE
#define SPI_BRIDGE_SPI_BASE LPSPI1
#endif

#if SPI_BRIDGE_ENABLE_DEBUG
#define SPI_BRIDGE_LOG_FAILURE(tag) SPI_BRIDGE_LOG("[spi-bridge] %s failed\r\n", tag)
#else
#define SPI_BRIDGE_LOG_FAILURE(tag) ((void)0)
#endif

#define SPI_BRIDGE_BLOCK_COUNT (1U + (SPI_BRIDGE_MAX_DEVICES * 2U))
#define SPI_BRIDGE_REGION_SIZE (SPI_BRIDGE_BLOCK_COUNT * SPI_BRIDGE_BLOCK_SIZE)

/* State mirrored over the SPI connection. */
static spi_bridge_block_t s_hubStatus;
static spi_bridge_block_t s_inBlocks[SPI_BRIDGE_MAX_DEVICES];
static spi_bridge_block_t s_outBlocks[SPI_BRIDGE_MAX_DEVICES];
static bool s_deviceInUse[SPI_BRIDGE_MAX_DEVICES];
static uint8_t s_connectedTable[SPI_BRIDGE_MAX_DEVICES];
static uint8_t s_txBuffer[SPI_BRIDGE_REGION_SIZE];
static uint8_t s_rxBuffer[SPI_BRIDGE_REGION_SIZE];

static uint8_t s_lastHubLoggedHeader;
static uint8_t s_lastInLoggedHeader[SPI_BRIDGE_MAX_DEVICES];
static uint8_t s_lastOutLoggedHeader[SPI_BRIDGE_MAX_DEVICES];

static spi_bridge_block_t s_lastLoggedHubStatus;
static spi_bridge_block_t s_lastLoggedInBlocks[SPI_BRIDGE_MAX_DEVICES];
static spi_bridge_block_t s_lastLoggedOutBlocks[SPI_BRIDGE_MAX_DEVICES];

static uint8_t SPI_BridgeGetDeviceAddress(uint8_t deviceId)
{
    return (uint8_t)(deviceId + 1U);
}

static uint16_t SPI_BridgeGetBlockAddress(uint8_t blockIndex)
{
    return (uint16_t)(blockIndex * SPI_BRIDGE_BLOCK_SIZE);
}

static uint8_t SPI_BridgeExtractLength(uint8_t header)
{
    return (uint8_t)((header & SPI_BRIDGE_HEADER_LENGTH_MASK) >> SPI_BRIDGE_HEADER_LENGTH_SHIFT);
}

static uint8_t SPI_BridgeMakeHeader(bool dirty, uint8_t type, uint8_t length)
{
    return (uint8_t)((dirty ? SPI_BRIDGE_HEADER_DIRTY_MASK : 0U) | (type ? SPI_BRIDGE_HEADER_TYPE_MASK : 0U) |
                     ((length << SPI_BRIDGE_HEADER_LENGTH_SHIFT) & SPI_BRIDGE_HEADER_LENGTH_MASK));
}

static uint16_t SPI_BridgeCrcUpdate(uint16_t crc, uint8_t data)
{
    crc ^= (uint16_t)data << 8U;
    for (int i = 0; i < 8; ++i)
    {
        if ((crc & 0x8000U) != 0U)
        {
            crc = (uint16_t)((crc << 1U) ^ SPI_BRIDGE_CRC_POLY);
        }
        else
        {
            crc <<= 1U;
        }
    }
    return crc;
}

static uint16_t SPI_BridgeComputeBlockCrc(const uint8_t *block)
{
    uint8_t header = block[0];
    uint8_t length = SPI_BridgeExtractLength(header);
    uint16_t crc   = SPI_BRIDGE_CRC_INIT;

    /* CRC covers the header and the active payload bytes. */
    crc = SPI_BridgeCrcUpdate(crc, header);
    for (uint8_t i = 0; i < length; ++i)
    {
        crc = SPI_BridgeCrcUpdate(crc, block[1U + i]);
    }

    return crc;
}

static void SPI_BridgeUpdateBlockCrc(spi_bridge_block_t *block)
{
    uint8_t raw[SPI_BRIDGE_BLOCK_SIZE];

    /* Serialize to the raw wire format before recomputing CRC. */
    raw[0] = block->header;
    (void)memcpy(&raw[1], block->payload, SPI_BRIDGE_MAX_PAYLOAD_LENGTH);
    block->crc = SPI_BridgeComputeBlockCrc(raw);
}

static void SPI_BridgeSerializeBlock(const spi_bridge_block_t *block, uint8_t *output)
{
    uint8_t length = SPI_BridgeExtractLength(block->header);

    /* Copy header and payload, append CRC, pad unused payload bytes with 0. */
    output[0] = block->header;
    (void)memcpy(&output[1], block->payload, SPI_BRIDGE_MAX_PAYLOAD_LENGTH);
    output[1U + length] = (uint8_t)(block->crc & 0xFFU);
    output[2U + length] = (uint8_t)(block->crc >> 8U);
    if ((3U + length) < SPI_BRIDGE_BLOCK_SIZE)
    {
        (void)memset(&output[3U + length], 0, SPI_BRIDGE_BLOCK_SIZE - (3U + length));
    }
}

static void SPI_BridgeMarkDirty(spi_bridge_block_t *block, bool dirty)
{
    if (dirty)
    {
        block->header |= SPI_BRIDGE_HEADER_DIRTY_MASK;
    }
    else
    {
        block->header &= (uint8_t)(~SPI_BRIDGE_HEADER_DIRTY_MASK);
    }
    SPI_BridgeUpdateBlockCrc(block);
}

static bool SPI_BridgeBlocksEqual(const spi_bridge_block_t *a, const spi_bridge_block_t *b)
{
    return (a->header == b->header) &&
           (memcmp(a->payload, b->payload, SPI_BRIDGE_MAX_PAYLOAD_LENGTH) == 0) &&
           (a->crc == b->crc);
}

static void SPI_BridgeLogHexBuffer(const uint8_t *data, uint8_t length)
{
    if (length == 0U)
    {
        SPI_BRIDGE_LOG("    (empty)\r\n");
        return;
    }

    for (uint8_t i = 0U; i < length; ++i)
    {
        SPI_BRIDGE_LOG("    %02x%s", data[i], (((i + 1U) % 16U) == 0U) ? "\r\n" : " ");
    }
    if ((length % 16U) != 0U)
    {
        SPI_BRIDGE_LOG("\r\n");
    }
}

static void SPI_BridgeLogBlock(const char *label, uint8_t blockIndex, const spi_bridge_block_t *block)
{
    uint8_t length = SPI_BridgeExtractLength(block->header);
    uint8_t serialized[SPI_BRIDGE_BLOCK_SIZE];

    /* Render the logical block and its on-the-wire representation. */
    SPI_BridgeSerializeBlock(block, serialized);
    SPI_BRIDGE_LOG("SPI_BRIDGE: [%s @0x%04X] DIRTY=%u TYPE=%u LEN=%u CRC=0x%04X\r\n", label,
                   SPI_BridgeGetBlockAddress(blockIndex),
                   (block->header & SPI_BRIDGE_HEADER_DIRTY_MASK) ? 1U : 0U,
                   (block->header & SPI_BRIDGE_HEADER_TYPE_MASK) ? 1U : 0U, length, block->crc);
    SPI_BRIDGE_LOG("SPI_BRIDGE: [%s] payload:\r\n", label);
    SPI_BridgeLogHexBuffer(block->payload, length);
    SPI_BRIDGE_LOG("SPI_BRIDGE: [%s] register image:\r\n", label);
    SPI_BridgeLogHexBuffer(serialized, (uint8_t)(3U + length));
}

static void SPI_BridgeSetBlockPayload(spi_bridge_block_t *block, uint8_t type, const uint8_t *payload, uint8_t length)
{
    uint8_t safeLength = (length > SPI_BRIDGE_MAX_PAYLOAD_LENGTH) ? SPI_BRIDGE_MAX_PAYLOAD_LENGTH : length;
    uint8_t cleanHeader = SPI_BridgeMakeHeader(false, type, safeLength);

    /* Replace payload, mark as dirty, and regenerate CRC. */
    block->header = cleanHeader;
    if (safeLength > 0U)
    {
        (void)memcpy(block->payload, payload, safeLength);
    }
    if (safeLength < SPI_BRIDGE_MAX_PAYLOAD_LENGTH)
    {
        (void)memset(&block->payload[safeLength], 0, SPI_BRIDGE_MAX_PAYLOAD_LENGTH - safeLength);
    }

    block->header = (uint8_t)(cleanHeader | SPI_BRIDGE_HEADER_DIRTY_MASK);
    SPI_BridgeUpdateBlockCrc(block);
}

static bool SPI_BridgeBlockHasValidCrc(const uint8_t *raw)
{
    uint16_t computed = SPI_BridgeComputeBlockCrc(raw);
    uint8_t length    = SPI_BridgeExtractLength(raw[0]);
    uint16_t received = (uint16_t)raw[1U + length] | ((uint16_t)raw[2U + length] << 8U);

    return computed == received;
}

static void SPI_BridgeLogGridRow(const char *label, uint8_t blockIndex, const spi_bridge_block_t *block)
{
    uint8_t length = SPI_BridgeExtractLength(block->header);

    SPI_BRIDGE_LOG("| %-9s | 0x%04X | %-5s |   %u  | %3u | 0x%04X |\r\n", label,
                   SPI_BridgeGetBlockAddress(blockIndex),
                   (block->header & SPI_BRIDGE_HEADER_DIRTY_MASK) ? "dirty" : "clean",
                   (block->header & SPI_BRIDGE_HEADER_TYPE_MASK) ? 1U : 0U, length, block->crc);
}

static void SPI_BridgeLogGrid(void)
{
    SPI_BRIDGE_LOG("SDI register grid:\r\n");
    SPI_BRIDGE_LOG("+-----------+-------+-------+------+-----+-------+\r\n");
    SPI_BRIDGE_LOG("| Block     | Addr  | Dirty | Type | Len |  CRC  |\r\n");
    SPI_BRIDGE_LOG("+-----------+-------+-------+------+-----+-------+\r\n");

    SPI_BridgeLogGridRow("HUB_STATUS", 0U, &s_hubStatus);

    for (uint8_t deviceId = 0; deviceId < SPI_BRIDGE_MAX_DEVICES; ++deviceId)
    {
        uint8_t deviceAddress = SPI_BridgeGetDeviceAddress(deviceId);
        char label[16];

        (void)snprintf(label, sizeof(label), "DEV_IN[%u]", deviceAddress);
        SPI_BridgeLogGridRow(label, (uint8_t)(1U + (deviceId * 2U)), &s_inBlocks[deviceId]);

        (void)snprintf(label, sizeof(label), "DEV_OUT[%u]", deviceAddress);
        SPI_BridgeLogGridRow(label, (uint8_t)(2U + (deviceId * 2U)), &s_outBlocks[deviceId]);
    }

    SPI_BRIDGE_LOG("+-----------+-------+-------+------+-----+-------+\r\n");
}

static bool SPI_BridgeLogHub(void)
{
    if (SPI_BridgeBlocksEqual(&s_hubStatus, &s_lastLoggedHubStatus))
    {
        return false;
    }

    SPI_BridgeLogBlock("HUB_STATUS", 0U, &s_hubStatus);
    s_lastLoggedHubStatus = s_hubStatus;
    s_lastHubLoggedHeader = s_hubStatus.header;

    return true;
}

static bool SPI_BridgeLogIn(uint8_t deviceId)
{
    if (deviceId >= SPI_BRIDGE_MAX_DEVICES)
    {
        return false;
    }

    if (SPI_BridgeBlocksEqual(&s_inBlocks[deviceId], &s_lastLoggedInBlocks[deviceId]))
    {
        return false;
    }

    uint8_t deviceAddress = SPI_BridgeGetDeviceAddress(deviceId);
    uint8_t blockIndex = 1U + (deviceId * 2U);
    char label[16];

    (void)snprintf(label, sizeof(label), "DEV_IN[%u]", deviceAddress);
    SPI_BridgeLogBlock(label, blockIndex, &s_inBlocks[deviceId]);
    s_lastInLoggedHeader[deviceId] = s_inBlocks[deviceId].header;
    s_lastLoggedInBlocks[deviceId] = s_inBlocks[deviceId];

    return true;
}

static bool SPI_BridgeLogOut(uint8_t deviceId, bool done)
{
    if (deviceId >= SPI_BRIDGE_MAX_DEVICES)
    {
        return false;
    }

    uint8_t header = s_outBlocks[deviceId].header;
    uint8_t deviceAddress = SPI_BridgeGetDeviceAddress(deviceId);
    uint8_t blockIndex = 2U + (deviceId * 2U);
    char label[16];

    if (!done && SPI_BridgeBlocksEqual(&s_outBlocks[deviceId], &s_lastLoggedOutBlocks[deviceId]))
    {
        return false;
    }

    (void)snprintf(label, sizeof(label), "DEV_OUT[%u]", deviceAddress);
    SPI_BridgeLogBlock(label, blockIndex, &s_outBlocks[deviceId]);
    s_lastOutLoggedHeader[deviceId] = header;
    s_lastLoggedOutBlocks[deviceId] = s_outBlocks[deviceId];

    return true;
}

static void SPI_BridgeLogState(void)
{
    bool stateChanged = SPI_BridgeLogHub();

    for (uint8_t deviceId = 0; deviceId < SPI_BRIDGE_MAX_DEVICES; ++deviceId)
    {
        stateChanged |= SPI_BridgeLogIn(deviceId);
        stateChanged |= SPI_BridgeLogOut(deviceId, false);
    }

    if (stateChanged)
    {
        SPI_BridgeLogGrid();
    }
}

static void SPI_BridgeRebuildHubStatus(void)
{
    uint8_t payloadLength = SPI_BRIDGE_MAX_DEVICES;
    uint8_t cleanHeader   = SPI_BridgeMakeHeader(false, 0U, payloadLength);

    /* Populate the hub bitmap payload then flag it dirty so the host reads it. */
    s_hubStatus.header = cleanHeader;
    (void)memcpy(s_hubStatus.payload, s_connectedTable, payloadLength);
    if (payloadLength < SPI_BRIDGE_MAX_PAYLOAD_LENGTH)
    {
        (void)memset(&s_hubStatus.payload[payloadLength], 0, SPI_BRIDGE_MAX_PAYLOAD_LENGTH - payloadLength);
    }

    s_hubStatus.header = (uint8_t)(cleanHeader | SPI_BRIDGE_HEADER_DIRTY_MASK);
    SPI_BridgeUpdateBlockCrc(&s_hubStatus);
}

static status_t SPI_BridgeTransferByte(LPSPI_Type *base, uint8_t txData, uint8_t *rxData)
{
    uint32_t timeout = SPI_BRIDGE_SPI_TIMEOUT;

    /* Wait for TX FIFO space then push a byte. */
    while (((base->SR & LPSPI_SR_TDF_MASK) == 0U) && (timeout-- != 0U))
    {
    }

    if (timeout == 0U)
    {
        return kStatus_Fail;
    }

    base->TDR = txData;

    timeout = SPI_BRIDGE_SPI_TIMEOUT;
    /* Wait for RX data (the peer's response). */
    while (((base->SR & LPSPI_SR_RDF_MASK) == 0U) && (timeout-- != 0U))
    {
    }

    if (timeout == 0U)
    {
        return kStatus_Fail;
    }

    *rxData = (uint8_t)base->RDR;
    base->SR = LPSPI_SR_RDF_MASK | LPSPI_SR_TDF_MASK;

    return kStatus_Success;
}

static status_t SPI_BridgeTransferRegion(const uint8_t *tx, uint8_t *rx, size_t length)
{
    for (size_t i = 0; i < length; ++i)
    {
        status_t status = SPI_BridgeTransferByte(SPI_BRIDGE_SPI_BASE, tx[i], &rx[i]);
        if (status != kStatus_Success)
        {
            return status;
        }
    }

    return kStatus_Success;
}

static void SPI_BridgeSerializeMap(uint8_t *txBuffer)
{
    uint8_t *cursor = txBuffer;

    /* Serialize hub + IN/OUT blocks sequentially into the transfer region. */
    SPI_BridgeSerializeBlock(&s_hubStatus, cursor);
    cursor += SPI_BRIDGE_BLOCK_SIZE;

    for (uint8_t deviceId = 0; deviceId < SPI_BRIDGE_MAX_DEVICES; ++deviceId)
    {
        SPI_BridgeSerializeBlock(&s_inBlocks[deviceId], cursor);
        cursor += SPI_BRIDGE_BLOCK_SIZE;
        SPI_BridgeSerializeBlock(&s_outBlocks[deviceId], cursor);
        cursor += SPI_BRIDGE_BLOCK_SIZE;
    }
}

static void SPI_BridgeProcessHubWrite(const uint8_t *rxBlock)
{
    if (!SPI_BridgeBlockHasValidCrc(rxBlock))
    {
        return;
    }

    if (((rxBlock[0] & SPI_BRIDGE_HEADER_DIRTY_MASK) == 0U) &&
        ((s_hubStatus.header & SPI_BRIDGE_HEADER_DIRTY_MASK) != 0U))
    {
        s_hubStatus.header &= (uint8_t)~SPI_BRIDGE_HEADER_DIRTY_MASK;
        SPI_BridgeUpdateBlockCrc(&s_hubStatus);
    }
}

static void SPI_BridgeProcessInWrite(uint8_t deviceId, const uint8_t *rxBlock)
{
    if ((deviceId >= SPI_BRIDGE_MAX_DEVICES) || !SPI_BridgeBlockHasValidCrc(rxBlock))
    {
        return;
    }

    if (((rxBlock[0] & SPI_BRIDGE_HEADER_DIRTY_MASK) == 0U) &&
        ((s_inBlocks[deviceId].header & SPI_BRIDGE_HEADER_DIRTY_MASK) != 0U))
    {
        s_inBlocks[deviceId].header &= (uint8_t)~SPI_BRIDGE_HEADER_DIRTY_MASK;
        SPI_BridgeUpdateBlockCrc(&s_inBlocks[deviceId]);
    }
}

static void SPI_BridgeProcessOutWrite(uint8_t deviceId, const uint8_t *rxBlock)
{
    if (deviceId >= SPI_BRIDGE_MAX_DEVICES)
    {
        return;
    }

    uint8_t header  = rxBlock[0];
    uint8_t length  = SPI_BridgeExtractLength(header);
    bool dirty      = (header & SPI_BRIDGE_HEADER_DIRTY_MASK) != 0U;

    if (!dirty || (length > SPI_BRIDGE_MAX_PAYLOAD_LENGTH))
    {
        return;
    }

    if (!SPI_BridgeBlockHasValidCrc(rxBlock))
    {
        return;
    }

    s_outBlocks[deviceId].header = header;
    (void)memcpy(s_outBlocks[deviceId].payload, &rxBlock[1], length);
    if (length < SPI_BRIDGE_MAX_PAYLOAD_LENGTH)
    {
        (void)memset(&s_outBlocks[deviceId].payload[length], 0, SPI_BRIDGE_MAX_PAYLOAD_LENGTH - length);
    }
    SPI_BridgeUpdateBlockCrc(&s_outBlocks[deviceId]);
    SPI_BridgeLogOut(deviceId, false);
}

static void SPI_BridgeProcessIncoming(const uint8_t *rxBuffer)
{
    const uint8_t *cursor = rxBuffer;

    /* Process hub, IN, and OUT regions in order. */
    SPI_BridgeProcessHubWrite(cursor);
    cursor += SPI_BRIDGE_BLOCK_SIZE;

    for (uint8_t deviceId = 0; deviceId < SPI_BRIDGE_MAX_DEVICES; ++deviceId)
    {
        SPI_BridgeProcessInWrite(deviceId, cursor);
        cursor += SPI_BRIDGE_BLOCK_SIZE;
        SPI_BridgeProcessOutWrite(deviceId, cursor);
        cursor += SPI_BRIDGE_BLOCK_SIZE;
    }
}

status_t SPI_BridgeInit(void)
{
    /* Configure LPSPI1 as an 8-bit slave before any transfers occur. */
    CLOCK_SetMux(kCLOCK_LpspiMux, 1U);
    CLOCK_SetDiv(kCLOCK_LpspiDiv, 7U);
    CLOCK_EnableClock(kCLOCK_Lpspi1);

    SPI_BRIDGE_SPI_BASE->CR = LPSPI_CR_RST_MASK;
    SPI_BRIDGE_SPI_BASE->CR = 0U;
    SPI_BRIDGE_SPI_BASE->CFGR1 = LPSPI_CFGR1_MASTER(0U) | LPSPI_CFGR1_NOSTALL(1U);
    SPI_BRIDGE_SPI_BASE->FCR   = LPSPI_FCR_TXWATER(0U) | LPSPI_FCR_RXWATER(0U);
    SPI_BRIDGE_SPI_BASE->TCR   = LPSPI_TCR_FRAMESZ(7U) | LPSPI_TCR_PCS(0U) | LPSPI_TCR_CPHA(0U) |
                               LPSPI_TCR_CPOL(0U);
    SPI_BRIDGE_SPI_BASE->CR    = LPSPI_CR_MEN_MASK;

    (void)memset(&s_hubStatus, 0, sizeof(s_hubStatus));
    (void)memset(s_inBlocks, 0, sizeof(s_inBlocks));
    (void)memset(s_outBlocks, 0, sizeof(s_outBlocks));
    (void)memset(s_deviceInUse, 0, sizeof(s_deviceInUse));
    (void)memset(s_connectedTable, 0, sizeof(s_connectedTable));
    (void)memset(s_lastInLoggedHeader, 0, sizeof(s_lastInLoggedHeader));
    (void)memset(s_lastOutLoggedHeader, 0, sizeof(s_lastOutLoggedHeader));
    (void)memset(&s_lastLoggedHubStatus, 0, sizeof(s_lastLoggedHubStatus));
    (void)memset(s_lastLoggedInBlocks, 0, sizeof(s_lastLoggedInBlocks));
    (void)memset(s_lastLoggedOutBlocks, 0, sizeof(s_lastLoggedOutBlocks));
    s_lastHubLoggedHeader = 0U;

    SPI_BridgeRebuildHubStatus();

    return kStatus_Success;
}

void SPI_BridgeTask(void *param)
{
    (void)param;

    while (1)
    {
        /* Push local state to the host and apply any incoming changes. */
        SPI_BridgeSerializeMap(s_txBuffer);
        if (SPI_BridgeTransferRegion(s_txBuffer, s_rxBuffer, SPI_BRIDGE_REGION_SIZE) == kStatus_Success)
        {
            SPI_BridgeProcessIncoming(s_rxBuffer);
            SPI_BridgeLogState();
        }
        else
        {
            SPI_BRIDGE_LOG_FAILURE("transfer");
        }
    }
}

static int32_t SPI_BridgeFindFreeSlot(void)
{
    for (uint8_t deviceId = 0; deviceId < SPI_BRIDGE_MAX_DEVICES; ++deviceId)
    {
        if (!s_deviceInUse[deviceId])
        {
            return deviceId;
        }
    }
    return -1;
}

status_t SPI_BridgeAllocDevice(uint8_t *deviceIdOut)
{
    int32_t slot = SPI_BridgeFindFreeSlot();

    if (slot < 0)
    {
        return kStatus_Fail;
    }

    s_deviceInUse[slot]      = true;
    s_connectedTable[slot]   = 1U;
    s_inBlocks[slot].header  = SPI_BridgeMakeHeader(false, 0U, 0U);
    s_outBlocks[slot].header = SPI_BridgeMakeHeader(false, 0U, 0U);
    SPI_BridgeUpdateBlockCrc(&s_inBlocks[slot]);
    SPI_BridgeUpdateBlockCrc(&s_outBlocks[slot]);

    SPI_BridgeRebuildHubStatus();

    if (deviceIdOut != NULL)
    {
        *deviceIdOut = (uint8_t)slot;
    }

    return kStatus_Success;
}

status_t SPI_BridgeRemoveDevice(uint8_t deviceId)
{
    if (deviceId >= SPI_BRIDGE_MAX_DEVICES)
    {
        return kStatus_InvalidArgument;
    }

    if (!s_deviceInUse[deviceId])
    {
        return kStatus_InvalidArgument;
    }

    s_deviceInUse[deviceId]    = false;
    s_connectedTable[deviceId] = 0U;
    (void)memset(&s_inBlocks[deviceId], 0, sizeof(s_inBlocks[deviceId]));
    (void)memset(&s_outBlocks[deviceId], 0, sizeof(s_outBlocks[deviceId]));

    SPI_BridgeRebuildHubStatus();

    return kStatus_Success;
}

status_t SPI_BridgeSendReportDescriptor(uint8_t deviceId, const uint8_t *descriptor, uint16_t length)
{
    if ((deviceId >= SPI_BRIDGE_MAX_DEVICES) || (descriptor == NULL) || (length > SPI_BRIDGE_MAX_PAYLOAD_LENGTH))
    {
        return kStatus_InvalidArgument;
    }

    SPI_BridgeSetBlockPayload(&s_inBlocks[deviceId], 1U, descriptor, (uint8_t)length);
    return kStatus_Success;
}

status_t SPI_BridgeSendReport(uint8_t deviceId, bool inDirection, uint8_t reportId, const uint8_t *data, uint16_t length)
{
    (void)reportId;
    if ((deviceId >= SPI_BRIDGE_MAX_DEVICES) || (data == NULL) || (length > SPI_BRIDGE_MAX_PAYLOAD_LENGTH))
    {
        return kStatus_InvalidArgument;
    }

    if (!inDirection)
    {
        return kStatus_InvalidArgument;
    }

    SPI_BridgeSetBlockPayload(&s_inBlocks[deviceId], 0U, data, (uint8_t)length);

    return kStatus_Success;
}

bool SPI_BridgeGetOutReport(uint8_t deviceId, uint8_t *typeOut, uint8_t *payloadOut, uint8_t *lengthOut)
{
    if (deviceId >= SPI_BRIDGE_MAX_DEVICES)
    {
        return false;
    }

    uint8_t header = s_outBlocks[deviceId].header;
    uint8_t length = SPI_BridgeExtractLength(header);

    if (((header & SPI_BRIDGE_HEADER_DIRTY_MASK) == 0U) || (length > SPI_BRIDGE_MAX_PAYLOAD_LENGTH))
    {
        return false;
    }

    if (payloadOut != NULL)
    {
        (void)memcpy(payloadOut, s_outBlocks[deviceId].payload, length);
    }
    if (typeOut != NULL)
    {
        *typeOut = (uint8_t)((header & SPI_BRIDGE_HEADER_TYPE_MASK) ? 1U : 0U);
    }
    if (lengthOut != NULL)
    {
        *lengthOut = length;
    }

    return true;
}

status_t SPI_BridgeClearOutReport(uint8_t deviceId)
{
    if (deviceId >= SPI_BRIDGE_MAX_DEVICES)
    {
        return kStatus_InvalidArgument;
    }

    SPI_BridgeMarkDirty(&s_outBlocks[deviceId], false);
    return kStatus_Success;
}

