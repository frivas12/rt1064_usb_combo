#include "spi_bridge.h"

#include "FreeRTOS.h"
#include "app.h"
#include "board.h"
#include "fsl_clock.h"
#include "pin_mux.h"
#include "usb_host.h"

#include <string.h>

#define SPI_BRIDGE_TX_HEADER_SIZE (sizeof(spi_bridge_header_t))
#define SPI_BRIDGE_SPI_TIMEOUT    (1000000U)

#ifndef SPI_BRIDGE_SPI_BASE
#define SPI_BRIDGE_SPI_BASE LPSPI1
#endif

#if SPI_BRIDGE_ENABLE_DEBUG
#define SPI_BRIDGE_LOG_FAILURE(tag) SPI_BRIDGE_LOG("[spi-bridge] %s failed\r\n", tag)
#else
#define SPI_BRIDGE_LOG_FAILURE(tag) ((void)0)
#endif

typedef struct _spi_bridge_frame
{
    spi_bridge_header_t header;
    uint16_t payloadLength;
    uint8_t payload[SPI_BRIDGE_MAX_PAYLOAD];
} spi_bridge_frame_t;

#if SPI_BRIDGE_ENABLE_DEBUG
static void SPI_BridgeDumpFrame(const spi_bridge_frame_t *frame)
{
    SPI_BRIDGE_LOG("[spi-bridge] tx frame: type=0x%02x device=%u len=%u seq=%u crc=0x%04x\r\n",
                   frame->header.msgType, frame->header.deviceId, frame->header.length, frame->header.seq,
                   frame->header.crc);

    if (frame->payloadLength == 0U)
    {
        return;
    }

    for (uint16_t offset = 0; offset < frame->payloadLength; offset += 16U)
    {
        uint16_t line = (uint16_t)((frame->payloadLength - offset) > 16U ? 16U : (frame->payloadLength - offset));
        SPI_BRIDGE_LOG("  %03u: ", (unsigned int)offset);
        for (uint16_t index = 0; index < line; ++index)
        {
            SPI_BRIDGE_LOG("%02x ", frame->payload[offset + index]);
        }
        SPI_BRIDGE_LOG("\r\n");
    }
}
#else
#define SPI_BridgeDumpFrame(frame) ((void)0)
#endif

typedef struct _spi_bridge_device_entry
{
    bool inUse;
    uint8_t deviceId;
    uint16_t vid;
    uint16_t pid;
    uint8_t interfaceNumber;
    uint8_t hasOutEndpoint;
    uint8_t deviceType;
    uint16_t reportDescLen;
    uint8_t hubNumber;
    uint8_t portNumber;
    uint8_t hsHubNumber;
    uint8_t hsHubPort;
    uint8_t level;
} spi_bridge_device_entry_t;

static QueueHandle_t s_bridgeQueue;
static spi_bridge_device_entry_t s_devices[SPI_BRIDGE_MAX_DEVICES];
static uint16_t s_txSeq;

static status_t SPI_BridgeWriteByte(LPSPI_Type *base, uint8_t data)
{
    uint32_t timeout = SPI_BRIDGE_SPI_TIMEOUT;

    while (((base->SR & LPSPI_SR_TDF_MASK) == 0U) && (timeout-- != 0U))
    {
    }

    if (timeout == 0U)
    {
        return kStatus_Fail;
    }

    base->TDR = data;

    timeout = SPI_BRIDGE_SPI_TIMEOUT;
    while (((base->SR & LPSPI_SR_RDF_MASK) == 0U) && (timeout-- != 0U))
    {
    }

    if (timeout == 0U)
    {
        return kStatus_Fail;
    }

    (void)base->RDR; /* clear received data */
    base->SR = LPSPI_SR_RDF_MASK | LPSPI_SR_TDF_MASK;

    return kStatus_Success;
}

static status_t SPI_BridgeWriteFrame(const spi_bridge_frame_t *frame)
{
    uint16_t index;
    const uint8_t *rawHeader = (const uint8_t *)&frame->header;
    status_t status;

    for (index = 0; index < SPI_BRIDGE_TX_HEADER_SIZE; ++index)
    {
        status = SPI_BridgeWriteByte(SPI_BRIDGE_SPI_BASE, rawHeader[index]);
        if (status != kStatus_Success)
        {
            return status;
        }
    }

    for (index = 0; index < frame->payloadLength; ++index)
    {
        status = SPI_BridgeWriteByte(SPI_BRIDGE_SPI_BASE, frame->payload[index]);
        if (status != kStatus_Success)
        {
            return status;
        }
    }

    return kStatus_Success;
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

static uint16_t SPI_BridgeComputeCrc(uint8_t msgType, uint8_t deviceId, uint16_t length, uint16_t seq,
                                     const uint8_t *payload, uint16_t payloadLength)
{
    uint16_t crc = SPI_BRIDGE_CRC_INIT;

    crc = SPI_BridgeCrcUpdate(crc, SPI_BRIDGE_VERSION);
    crc = SPI_BridgeCrcUpdate(crc, msgType);
    crc = SPI_BridgeCrcUpdate(crc, deviceId);
    crc = SPI_BridgeCrcUpdate(crc, (uint8_t)(length & 0xFFU));
    crc = SPI_BridgeCrcUpdate(crc, (uint8_t)(length >> 8U));
    crc = SPI_BridgeCrcUpdate(crc, (uint8_t)(seq & 0xFFU));
    crc = SPI_BridgeCrcUpdate(crc, (uint8_t)(seq >> 8U));

    for (uint16_t i = 0; i < payloadLength; ++i)
    {
        crc = SPI_BridgeCrcUpdate(crc, payload[i]);
    }

    return crc;
}

static status_t SPI_BridgeQueueFrame(spi_bridge_message_type_t type, uint8_t deviceId, const uint8_t *payload,
                                     uint16_t length)
{
    spi_bridge_frame_t frame;

    if (length > SPI_BRIDGE_MAX_PAYLOAD)
    {
        return kStatus_InvalidArgument;
    }

    if (s_bridgeQueue == NULL)
    {
        return kStatus_Fail;
    }

    frame.header.sync    = SPI_BRIDGE_SYNC_BYTE;
    frame.header.version = SPI_BRIDGE_VERSION;
    frame.header.msgType = (uint8_t)type;
    frame.header.deviceId = deviceId;
    frame.header.length  = length;
    frame.header.seq     = s_txSeq++;
    frame.payloadLength  = length;

    if ((payload != NULL) && (length > 0U))
    {
        (void)memcpy(frame.payload, payload, length);
    }
    else if (length > 0U)
    {
        (void)memset(frame.payload, 0, length);
    }

    frame.header.crc = SPI_BridgeComputeCrc(frame.header.msgType, frame.header.deviceId, frame.header.length,
                                            frame.header.seq, frame.payload, frame.payloadLength);

    SPI_BridgeDumpFrame(&frame);

    if (xQueueSend(s_bridgeQueue, &frame, 0) == pdPASS)
    {
        return kStatus_Success;
    }

    return kStatus_Fail;
}

static spi_bridge_device_entry_t *SPI_BridgeAllocateEntry(void)
{
    for (uint8_t i = 0; i < SPI_BRIDGE_MAX_DEVICES; ++i)
    {
        if (!s_devices[i].inUse)
        {
            s_devices[i].deviceId = i;
            return &s_devices[i];
        }
    }
    return NULL;
}

static spi_bridge_device_entry_t *SPI_BridgeFindDevice(uint8_t deviceId)
{
    if (deviceId >= SPI_BRIDGE_MAX_DEVICES)
    {
        return NULL;
    }

    if (s_devices[deviceId].inUse)
    {
        return &s_devices[deviceId];
    }

    return NULL;
}

status_t SPI_BridgeInit(void)
{
    /* Enable the LPSPI1 clock and select a safe divider. */
    CLOCK_SetMux(kCLOCK_LpspiMux, 1U);
    CLOCK_SetDiv(kCLOCK_LpspiDiv, 7U);
    CLOCK_EnableClock(kCLOCK_Lpspi1);

    /* Reset and configure LPSPI1 for slave mode, 8-bit frames. */
    SPI_BRIDGE_SPI_BASE->CR = LPSPI_CR_RST_MASK;
    SPI_BRIDGE_SPI_BASE->CR = 0U;
    SPI_BRIDGE_SPI_BASE->CFGR1 = LPSPI_CFGR1_MASTER(0U) | LPSPI_CFGR1_NOSTALL(1U);
    SPI_BRIDGE_SPI_BASE->FCR   = LPSPI_FCR_TXWATER(0U) | LPSPI_FCR_RXWATER(0U);
    SPI_BRIDGE_SPI_BASE->TCR   = LPSPI_TCR_FRAMESZ(7U) | LPSPI_TCR_PCS(0U) | LPSPI_TCR_CPHA(0U) |
                               LPSPI_TCR_CPOL(0U);
    SPI_BRIDGE_SPI_BASE->CR    = LPSPI_CR_MEN_MASK;

    (void)memset(s_devices, 0, sizeof(s_devices));
    s_txSeq = 0U;

    s_bridgeQueue = xQueueCreate(SPI_BRIDGE_QUEUE_DEPTH, sizeof(spi_bridge_frame_t));
    if (s_bridgeQueue == NULL)
    {
        SPI_BRIDGE_LOG_FAILURE("queue create");
        return kStatus_Fail;
    }

    return kStatus_Success;
}

void SPI_BridgeTask(void *param)
{
    spi_bridge_frame_t frame;
    (void)param;

    while (1)
    {
        if (xQueueReceive(s_bridgeQueue, &frame, portMAX_DELAY) == pdPASS)
        {
            if (SPI_BridgeWriteFrame(&frame) != kStatus_Success)
            {
                SPI_BRIDGE_LOG_FAILURE("write frame");
            }
        }
    }
}

status_t SPI_BridgeAllocDevice(spi_bridge_device_type_t deviceType,
                               uint16_t vid,
                               uint16_t pid,
                               uint8_t interfaceNumber,
                               bool hasOutEndpoint,
                               uint16_t reportDescLen,
                               uint8_t hubNumber,
                               uint8_t portNumber,
                               uint8_t hsHubNumber,
                               uint8_t hsHubPort,
                               uint8_t level,
                               uint8_t *deviceIdOut)
{
    spi_bridge_device_entry_t *entry = SPI_BridgeAllocateEntry();
    spi_bridge_device_add_t payload;

    if (entry == NULL)
    {
        return kStatus_Fail;
    }

    entry->vid             = vid;
    entry->pid             = pid;
    entry->interfaceNumber = interfaceNumber;
    entry->hasOutEndpoint  = hasOutEndpoint ? 1U : 0U;
    entry->deviceType      = (uint8_t)deviceType;
    entry->reportDescLen   = reportDescLen;
    entry->hubNumber       = hubNumber;
    entry->portNumber      = portNumber;
    entry->hsHubNumber     = hsHubNumber;
    entry->hsHubPort       = hsHubPort;
    entry->level           = level;

    SPI_BRIDGE_LOG("[spi-bridge] allocate device %u: type=%u vid=0x%x pid=0x%x iface=%u outEp=%u reportLen=%u hub=%u port=%u hsHub=%u hsPort=%u level=%u\r\n",
             entry->deviceId, deviceType, vid, pid, interfaceNumber, entry->hasOutEndpoint, reportDescLen, hubNumber,
             portNumber, hsHubNumber, hsHubPort, level);

    payload.vid             = vid;
    payload.pid             = pid;
    payload.interfaceNumber = interfaceNumber;
    payload.hasOutEndpoint  = entry->hasOutEndpoint;
    payload.deviceType      = entry->deviceType;
    payload.reportDescLen   = reportDescLen;
    payload.hubNumber       = hubNumber;
    payload.portNumber      = portNumber;
    payload.hsHubNumber     = hsHubNumber;
    payload.hsHubPort       = hsHubPort;
    payload.level           = level;

    if (SPI_BridgeQueueFrame(kSpiBridgeMessageDeviceAdd, entry->deviceId, (const uint8_t *)&payload,
                             (uint16_t)sizeof(payload)) == kStatus_Success)
    {
        entry->inUse = true;
        if (deviceIdOut != NULL)
        {
            *deviceIdOut = entry->deviceId;
        }
        return kStatus_Success;
    }

    SPI_BRIDGE_LOG("[spi-bridge] failed to queue device add for %u\r\n", entry->deviceId);

    (void)memset(entry, 0, sizeof(*entry));
    return kStatus_Fail;
}

status_t SPI_BridgeRemoveDevice(uint8_t deviceId)
{
    spi_bridge_device_entry_t *entry = SPI_BridgeFindDevice(deviceId);

    if (entry == NULL)
    {
        return kStatus_InvalidArgument;
    }

    if (SPI_BridgeQueueFrame(kSpiBridgeMessageDeviceRemove, deviceId, NULL, 0U) == kStatus_Success)
    {
        (void)memset(entry, 0, sizeof(*entry));
        return kStatus_Success;
    }

    return kStatus_Fail;
}

status_t SPI_BridgeSendReportDescriptor(uint8_t deviceId,
                                        const uint8_t *descriptor,
                                        uint16_t totalLength,
                                        uint16_t offset,
                                        uint16_t chunkLength)
{
    spi_bridge_device_entry_t *entry = SPI_BridgeFindDevice(deviceId);
    spi_bridge_report_descriptor_t *payload;
    uint8_t buffer[SPI_BRIDGE_MAX_PAYLOAD];

    if ((entry == NULL) || (descriptor == NULL))
    {
        return kStatus_InvalidArgument;
    }

    if ((sizeof(*payload) + chunkLength) > sizeof(buffer))
    {
        return kStatus_InvalidArgument;
    }

    payload                = (spi_bridge_report_descriptor_t *)buffer;
    payload->totalLength   = totalLength;
    payload->offset        = offset;
    payload->chunkLength   = chunkLength;
    (void)memcpy(payload->data, descriptor, chunkLength);

    return SPI_BridgeQueueFrame(kSpiBridgeMessageReportDescriptor, entry->deviceId, buffer,
                                (uint16_t)(sizeof(*payload) + chunkLength));
}

status_t SPI_BridgeSendReport(uint8_t deviceId, bool inDirection, uint8_t reportId, const uint8_t *data, uint16_t length)
{
    spi_bridge_device_entry_t *entry = SPI_BridgeFindDevice(deviceId);
    spi_bridge_report_t *payload;
    uint8_t buffer[SPI_BRIDGE_MAX_PAYLOAD];

    if ((entry == NULL) || (data == NULL))
    {
        return kStatus_InvalidArgument;
    }

    if ((sizeof(*payload) + length) > sizeof(buffer))
    {
        return kStatus_InvalidArgument;
    }

    payload              = (spi_bridge_report_t *)buffer;
    payload->reportId    = reportId;
    payload->reportLength = length;
    (void)memcpy(payload->data, data, length);

    return SPI_BridgeQueueFrame(inDirection ? kSpiBridgeMessageReportIn : kSpiBridgeMessageReportOut,
                                entry->deviceId, buffer, (uint16_t)(sizeof(*payload) + length));
}

status_t SPI_BridgeSendCdcData(uint8_t deviceId, bool inDirection, const uint8_t *data, uint16_t length)
{
    spi_bridge_device_entry_t *entry = SPI_BridgeFindDevice(deviceId);
    spi_bridge_cdc_data_t *payload;
    uint8_t buffer[SPI_BRIDGE_MAX_PAYLOAD];

    if ((entry == NULL) || (data == NULL))
    {
        return kStatus_InvalidArgument;
    }

    if ((sizeof(*payload) + length) > sizeof(buffer))
    {
        return kStatus_InvalidArgument;
    }

    payload            = (spi_bridge_cdc_data_t *)buffer;
    payload->dataLength = length;
    (void)memcpy(payload->data, data, length);

    return SPI_BridgeQueueFrame(inDirection ? kSpiBridgeMessageCdcDataIn : kSpiBridgeMessageCdcDataOut,
                                entry->deviceId, buffer, (uint16_t)(sizeof(*payload) + length));
}
