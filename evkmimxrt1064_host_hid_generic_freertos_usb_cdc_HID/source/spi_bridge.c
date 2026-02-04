/*
 * Minimal SPI bridge test
 *
 * The RT1064 runs as an SPI slave on LPSPI1. Each received byte updates the
 * next transmit byte so the master can clock a reply on the following byte.
 */
#include "spi_bridge.h"

#include "FreeRTOS.h"
#include "task.h"
#include "fsl_clock.h"

#define SPI_BRIDGE_IDLE_TX (0x00U)

#ifndef SPI_BRIDGE_SPI_BASE
#define SPI_BRIDGE_SPI_BASE LPSPI1
#endif

static volatile uint8_t s_lastRxByte;
static volatile uint8_t s_nextTxByte = SPI_BRIDGE_IDLE_TX;

status_t SPI_BridgeInit(void)
{
    CLOCK_SetMux(kCLOCK_LpspiMux, 1U);
    CLOCK_SetDiv(kCLOCK_LpspiDiv, 7U);
    CLOCK_EnableClock(kCLOCK_Lpspi1);

    SPI_BRIDGE_SPI_BASE->CR = LPSPI_CR_RST_MASK;
    SPI_BRIDGE_SPI_BASE->CR = 0U;
    SPI_BRIDGE_SPI_BASE->CFGR1 =
        LPSPI_CFGR1_MASTER(0U) | LPSPI_CFGR1_NOSTALL(1U) | LPSPI_CFGR1_PINCFG(0U);
    SPI_BRIDGE_SPI_BASE->FCR = LPSPI_FCR_TXWATER(0U) | LPSPI_FCR_RXWATER(0U);
    SPI_BRIDGE_SPI_BASE->TCR =
        LPSPI_TCR_FRAMESZ(7U) | LPSPI_TCR_PCS(0U) | LPSPI_TCR_CPHA(0U) | LPSPI_TCR_CPOL(0U);
    SPI_BRIDGE_SPI_BASE->SR =
        LPSPI_SR_WCF_MASK | LPSPI_SR_FCF_MASK | LPSPI_SR_TCF_MASK | LPSPI_SR_TEF_MASK | LPSPI_SR_REF_MASK |
        LPSPI_SR_DMF_MASK;

    NVIC_SetPriority(LPSPI1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ(LPSPI1_IRQn);

    while ((SPI_BRIDGE_SPI_BASE->SR & LPSPI_SR_RDF_MASK) != 0U)
    {
        (void)SPI_BRIDGE_SPI_BASE->RDR;
    }

    s_nextTxByte = SPI_BRIDGE_IDLE_TX;
    SPI_BRIDGE_SPI_BASE->TDR = s_nextTxByte;
    SPI_BRIDGE_SPI_BASE->IER = LPSPI_IER_RDIE_MASK;
    SPI_BRIDGE_SPI_BASE->CR = LPSPI_CR_MEN_MASK;

    return kStatus_Success;
}

void SPI_BridgeTask(void *param)
{
    (void)param;

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(100U));
    }
}

void LPSPI1_IRQHandler(void)
{
    uint32_t sr = SPI_BRIDGE_SPI_BASE->SR;

    if ((sr & (LPSPI_SR_TEF_MASK | LPSPI_SR_REF_MASK)) != 0U)
    {
        SPI_BRIDGE_SPI_BASE->SR = LPSPI_SR_TEF_MASK | LPSPI_SR_REF_MASK;
    }

    while ((SPI_BRIDGE_SPI_BASE->SR & LPSPI_SR_RDF_MASK) != 0U)
    {
        uint8_t rx = (uint8_t)SPI_BRIDGE_SPI_BASE->RDR;
        s_lastRxByte = rx;
        s_nextTxByte = (uint8_t)(rx + 1U);
        SPI_BRIDGE_SPI_BASE->TDR = s_nextTxByte;
        SPI_BRIDGE_SPI_BASE->SR = LPSPI_SR_RDF_MASK | LPSPI_SR_TDF_MASK;
    }
}

status_t SPI_BridgeAllocDevice(uint8_t *deviceIdOut)
{
    if (deviceIdOut != NULL)
    {
        *deviceIdOut = 0U;
    }
    return kStatus_Success;
}

status_t SPI_BridgeRemoveDevice(uint8_t deviceId)
{
    (void)deviceId;
    return kStatus_Success;
}

status_t SPI_BridgeSendReportDescriptor(uint8_t deviceId, const uint8_t *descriptor, uint16_t length)
{
    (void)deviceId;
    (void)descriptor;
    (void)length;
    return kStatus_Success;
}

status_t SPI_BridgeSendReport(uint8_t deviceId, bool inDirection, uint8_t reportId, const uint8_t *data, uint16_t length)
{
    (void)deviceId;
    (void)inDirection;
    (void)reportId;
    (void)data;
    (void)length;
    return kStatus_Success;
}

status_t SPI_BridgeSendCdcIn(const uint8_t *data, uint8_t length)
{
    (void)data;
    (void)length;
    return kStatus_Success;
}

status_t SPI_BridgeSendCdcDescriptor(const uint8_t *descriptor, uint8_t length)
{
    (void)descriptor;
    (void)length;
    return kStatus_Success;
}

bool SPI_BridgeGetOutReport(uint8_t deviceId, uint8_t *typeOut, uint8_t *payloadOut, uint8_t *lengthOut)
{
    (void)deviceId;
    (void)typeOut;
    (void)payloadOut;
    (void)lengthOut;
    return false;
}

status_t SPI_BridgeClearOutReport(uint8_t deviceId)
{
    (void)deviceId;
    return kStatus_Success;
}

bool SPI_BridgeGetCdcOut(uint8_t *typeOut, uint8_t *payloadOut, uint8_t *lengthOut)
{
    (void)typeOut;
    (void)payloadOut;
    (void)lengthOut;
    return false;
}

status_t SPI_BridgeClearCdcOut(void)
{
    return kStatus_Success;
}

void SPI_BridgeSetStateTraceEnabled(bool enabled)
{
    (void)enabled;
}

bool SPI_BridgeStateTraceEnabled(void)
{
    return false;
}

void SPI_BridgeEnableFixedCdcResponse(void)
{
}
