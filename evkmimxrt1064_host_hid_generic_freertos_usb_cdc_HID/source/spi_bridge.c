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

#define SPI_BRIDGE_LOG_FAILURE(tag) usb_echo("[spi-bridge] %s failed\r\n", tag)

typedef struct _spi_bridge_frame
{
    spi_bridge_header_t header;
    uint16_t payloadLength;
    uint8_t payload[SPI_BRIDGE_MAX_PAYLOAD];
} spi_bridge_frame_t;

static QueueHandle_t s_bridgeQueue;

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

static status_t SPI_BridgeQueueFrame(spi_bridge_message_type_t type, uint8_t source, const uint8_t *payload,
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

    frame.header.messageType = (uint8_t)type;
    frame.header.source      = source;
    frame.header.length      = length;
    frame.payloadLength      = length;

    if ((payload != NULL) && (length > 0U))
    {
        (void)memcpy(frame.payload, payload, length);
    }

    if (xQueueSend(s_bridgeQueue, &frame, 0) == pdPASS)
    {
        return kStatus_Success;
    }

    return kStatus_Fail;
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

status_t SPI_BridgeSendDeviceEvent(bool attached, uint16_t vid, uint16_t pid)
{
    uint8_t payload[4];

    payload[0] = (uint8_t)(vid & 0xFFU);
    payload[1] = (uint8_t)((vid >> 8U) & 0xFFU);
    payload[2] = (uint8_t)(pid & 0xFFU);
    payload[3] = (uint8_t)((pid >> 8U) & 0xFFU);

    return SPI_BridgeQueueFrame(attached ? kSpiBridgeMessageAttach : kSpiBridgeMessageDetach, 0U, payload,
                                (uint16_t)sizeof(payload));
}

status_t SPI_BridgeSendReportDescriptor(const uint8_t *descriptor, uint16_t length)
{
    return SPI_BridgeQueueFrame(kSpiBridgeMessageReportDescriptor, 0U, descriptor, length);
}

status_t SPI_BridgeSendReport(bool inDirection, const uint8_t *data, uint16_t length)
{
    return SPI_BridgeQueueFrame(inDirection ? kSpiBridgeMessageReportIn : kSpiBridgeMessageReportOut,
                                inDirection ? USB_IN : USB_OUT, data, length);
}
