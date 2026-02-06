/*
 * SPI bring-up test (RT1064 slave side)
 *
 * Fixed 5-byte full-duplex frames with deterministic pattern generation
 * and double-buffered TX/RX data ownership for DMA-style handoff.
 */
#include "spi_bridge.h"

#include <string.h>

#include "FreeRTOS.h"
#include "fsl_clock.h"
#include "task.h"

#define FRAME_SIZE (SPI_BRIDGE_FRAME_SIZE)
#define SPI_BRIDGE_DMA_BASE DMA0
#define SPI_BRIDGE_DMAMUX_BASE DMAMUX
#define SPI_BRIDGE_DMA_RX_CHANNEL (0U)
#define SPI_BRIDGE_DMA_TX_CHANNEL (1U)
#define SPI_BRIDGE_DMA_RX_REQUEST (13U) /* kDmaRequestMuxLPSPI1Rx in PERI_DMAMUX.h */
#define SPI_BRIDGE_DMA_TX_REQUEST (14U) /* kDmaRequestMuxLPSPI1Tx in PERI_DMAMUX.h */
#define SPI_BRIDGE_FRAME_BITS (8U)

#ifndef SPI_BRIDGE_SPI_BASE
#define SPI_BRIDGE_SPI_BASE LPSPI1
#endif

volatile bool last_rx_good = false;
volatile uint32_t good_count = 0U;
volatile uint32_t bad_count = 0U;

volatile uint8_t last_rx[FRAME_SIZE];
volatile uint8_t last_tx[FRAME_SIZE];

static uint8_t tx_buf[2][FRAME_SIZE];
static uint8_t rx_buf[2][FRAME_SIZE];
static volatile uint8_t active_idx = 0U;
static volatile bool completed_pending = false;
static volatile uint8_t completed_idx = 0U;
static volatile uint32_t dropped_completions = 0U;

static TaskHandle_t s_bridgeTaskHandle;

static void dma_configure_lpspi_channels(void)
{
    CLOCK_EnableClock(kCLOCK_Dma);
#if defined(kCLOCK_Dmamux)
    CLOCK_EnableClock(kCLOCK_Dmamux);
#endif

    SPI_BRIDGE_DMAMUX_BASE->CHCFG[SPI_BRIDGE_DMA_RX_CHANNEL] = 0U;
    SPI_BRIDGE_DMAMUX_BASE->CHCFG[SPI_BRIDGE_DMA_TX_CHANNEL] = 0U;

    SPI_BRIDGE_DMAMUX_BASE->CHCFG[SPI_BRIDGE_DMA_RX_CHANNEL] =
        DMAMUX_CHCFG_SOURCE(SPI_BRIDGE_DMA_RX_REQUEST) | DMAMUX_CHCFG_ENBL_MASK;
    SPI_BRIDGE_DMAMUX_BASE->CHCFG[SPI_BRIDGE_DMA_TX_CHANNEL] =
        DMAMUX_CHCFG_SOURCE(SPI_BRIDGE_DMA_TX_REQUEST) | DMAMUX_CHCFG_ENBL_MASK;

    SPI_BRIDGE_DMA_BASE->CERQ = DMA_CERQ_CERQ(SPI_BRIDGE_DMA_RX_CHANNEL);
    SPI_BRIDGE_DMA_BASE->CERQ = DMA_CERQ_CERQ(SPI_BRIDGE_DMA_TX_CHANNEL);
    SPI_BRIDGE_DMA_BASE->CINT = DMA_CINT_CINT(SPI_BRIDGE_DMA_RX_CHANNEL);
    SPI_BRIDGE_DMA_BASE->CINT = DMA_CINT_CINT(SPI_BRIDGE_DMA_TX_CHANNEL);
    SPI_BRIDGE_DMA_BASE->CDNE = DMA_CDNE_CDNE(SPI_BRIDGE_DMA_RX_CHANNEL);
    SPI_BRIDGE_DMA_BASE->CDNE = DMA_CDNE_CDNE(SPI_BRIDGE_DMA_TX_CHANNEL);

    NVIC_ClearPendingIRQ(DMA0_DMA16_IRQn);
    NVIC_SetPriority(DMA0_DMA16_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1U);
    EnableIRQ(DMA0_DMA16_IRQn);
}

static const uint8_t s_tx_const = 0x03U;
static const uint8_t s_expected_rx = 0x05U;

static void build_pattern_frame(uint8_t frame[FRAME_SIZE])
{
    for (uint32_t i = 0U; i < FRAME_SIZE; i++)
    {
        frame[i] = s_tx_const;
    }
}

static bool frame_matches_tx_pattern(const uint8_t rx[FRAME_SIZE], const uint8_t tx[FRAME_SIZE])
{
    (void)tx;
    for (uint32_t i = 0U; i < FRAME_SIZE; i++)
    {
        if (rx[i] != s_expected_rx)
        {
            return false;
        }
    }

    return true;
}

static void lpspi_slave_init_with_dma(void)
{
    CLOCK_SetMux(kCLOCK_LpspiMux, 1U);
    CLOCK_SetDiv(kCLOCK_LpspiDiv, 7U);
    CLOCK_EnableClock(kCLOCK_Lpspi1);

    SPI_BRIDGE_SPI_BASE->CR = LPSPI_CR_RST_MASK;
    SPI_BRIDGE_SPI_BASE->CR = 0U;
    SPI_BRIDGE_SPI_BASE->CR = LPSPI_CR_RRF_MASK | LPSPI_CR_RTF_MASK;
    SPI_BRIDGE_SPI_BASE->CFGR1 = LPSPI_CFGR1_MASTER(0U) | LPSPI_CFGR1_NOSTALL(0U) | LPSPI_CFGR1_PINCFG(0U);
    SPI_BRIDGE_SPI_BASE->FCR = LPSPI_FCR_TXWATER(0U) | LPSPI_FCR_RXWATER(0U);
    /*
     * FRAMESZ is encoded as (bits-per-frame - 1).
     * For 8-bit SPI transfers use FRAMESZ(7U), not FRAMESZ(8U).
     */
    SPI_BRIDGE_SPI_BASE->TCR = LPSPI_TCR_FRAMESZ(SPI_BRIDGE_FRAME_BITS - 1U) | LPSPI_TCR_PCS(0U) |
                               LPSPI_TCR_CPHA(0U) | LPSPI_TCR_CPOL(0U);
    SPI_BRIDGE_SPI_BASE->SR = LPSPI_SR_WCF_MASK | LPSPI_SR_FCF_MASK | LPSPI_SR_TCF_MASK | LPSPI_SR_TEF_MASK |
                              LPSPI_SR_REF_MASK | LPSPI_SR_DMF_MASK;

    /* DMA hookups are project-specific; these requests enable peripheral DMA. */
    SPI_BRIDGE_SPI_BASE->DER = LPSPI_DER_TDDE_MASK | LPSPI_DER_RDDE_MASK;
    SPI_BRIDGE_SPI_BASE->CR = LPSPI_CR_MEN_MASK;
}

static void arm_lpspi_dma(uint8_t *tx, uint8_t *rx)
{
    const uint32_t tx_dma_count = FRAME_SIZE;

    SPI_BRIDGE_DMA_BASE->CERQ = DMA_CERQ_CERQ(SPI_BRIDGE_DMA_RX_CHANNEL);
    SPI_BRIDGE_DMA_BASE->CERQ = DMA_CERQ_CERQ(SPI_BRIDGE_DMA_TX_CHANNEL);

    /*
     * Re-arm from a clean FIFO state to avoid bit/byte slip between frames.
     * Any stale RX residue from the previous frame can desynchronize the next
     * DMA transaction if it is not cleared before re-enabling requests.
     */
    SPI_BRIDGE_SPI_BASE->CR |= (LPSPI_CR_RRF_MASK | LPSPI_CR_RTF_MASK);

    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_RX_CHANNEL].SADDR = (uint32_t)&SPI_BRIDGE_SPI_BASE->RDR;
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_RX_CHANNEL].SOFF = 0;
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_RX_CHANNEL].ATTR = DMA_ATTR_SSIZE(0U) | DMA_ATTR_DSIZE(0U);
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_RX_CHANNEL].NBYTES_MLNO = 1U;
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_RX_CHANNEL].SLAST = 0;
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_RX_CHANNEL].DADDR = (uint32_t)rx;
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_RX_CHANNEL].DOFF = 1;
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_RX_CHANNEL].CITER_ELINKNO = DMA_CITER_ELINKNO_CITER(FRAME_SIZE);
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_RX_CHANNEL].DLAST_SGA = -(int32_t)FRAME_SIZE;
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_RX_CHANNEL].CSR = DMA_CSR_INTMAJOR_MASK | DMA_CSR_DREQ_MASK;
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_RX_CHANNEL].BITER_ELINKNO = DMA_BITER_ELINKNO_BITER(FRAME_SIZE);

    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_TX_CHANNEL].SADDR = (uint32_t)tx;
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_TX_CHANNEL].SOFF = 1;
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_TX_CHANNEL].ATTR = DMA_ATTR_SSIZE(0U) | DMA_ATTR_DSIZE(0U);
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_TX_CHANNEL].NBYTES_MLNO = 1U;
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_TX_CHANNEL].SLAST = -(int32_t)FRAME_SIZE;
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_TX_CHANNEL].DADDR = (uint32_t)&SPI_BRIDGE_SPI_BASE->TDR;
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_TX_CHANNEL].DOFF = 0;
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_TX_CHANNEL].CITER_ELINKNO = DMA_CITER_ELINKNO_CITER(tx_dma_count);
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_TX_CHANNEL].DLAST_SGA = 0;
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_TX_CHANNEL].CSR = DMA_CSR_DREQ_MASK;
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_TX_CHANNEL].BITER_ELINKNO = DMA_BITER_ELINKNO_BITER(tx_dma_count);

    SPI_BRIDGE_DMA_BASE->CINT = DMA_CINT_CINT(SPI_BRIDGE_DMA_RX_CHANNEL);
    SPI_BRIDGE_DMA_BASE->CINT = DMA_CINT_CINT(SPI_BRIDGE_DMA_TX_CHANNEL);
    SPI_BRIDGE_DMA_BASE->CDNE = DMA_CDNE_CDNE(SPI_BRIDGE_DMA_RX_CHANNEL);
    SPI_BRIDGE_DMA_BASE->CDNE = DMA_CDNE_CDNE(SPI_BRIDGE_DMA_TX_CHANNEL);

    if (tx_dma_count != 0U)
    {
        SPI_BRIDGE_DMA_BASE->SERQ = DMA_SERQ_SERQ(SPI_BRIDGE_DMA_TX_CHANNEL);
    }
    SPI_BRIDGE_DMA_BASE->SERQ = DMA_SERQ_SERQ(SPI_BRIDGE_DMA_RX_CHANNEL);
}

static void notify_processing_task_from_isr(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (s_bridgeTaskHandle != NULL)
    {
        vTaskNotifyGiveFromISR(s_bridgeTaskHandle, &xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static bool completed_fetch(uint8_t *idx)
{
    bool hasItem;

    taskENTER_CRITICAL();
    hasItem = completed_pending;
    if (hasItem)
    {
        *idx = completed_idx;
        completed_pending = false;
    }
    taskEXIT_CRITICAL();

    return hasItem;
}

static void rt1064_spi_bringup_process(uint8_t idx)
{
    uint8_t prep_idx = active_idx; /* prepare the buffer currently armed for next DMA transfer */

    memcpy((void *)last_rx, rx_buf[idx], FRAME_SIZE);
    memcpy((void *)last_tx, tx_buf[idx], FRAME_SIZE);

    last_rx_good = frame_matches_tx_pattern((const uint8_t *)last_rx, (const uint8_t *)last_tx);
    if (last_rx_good)
    {
        good_count++;
    }
    else
    {
        bad_count++;
    }

    build_pattern_frame(tx_buf[prep_idx]);
}

/*
 * Hook this function from the RX DMA completion IRQ in the selected
 * eDMA/channel configuration.
 */
void LPSPI_RX_DMA_IRQHandler(void)
{
    uint8_t finished_idx = active_idx;
    active_idx ^= 1U;

    arm_lpspi_dma(tx_buf[active_idx], rx_buf[active_idx]);
    if (completed_pending)
    {
        dropped_completions++;
    }
    completed_idx = finished_idx;
    completed_pending = true;
    notify_processing_task_from_isr();
}

void DMA0_DMA16_DriverIRQHandler(void)
{
    if ((SPI_BRIDGE_DMA_BASE->INT & (1UL << SPI_BRIDGE_DMA_RX_CHANNEL)) != 0UL)
    {
        SPI_BRIDGE_DMA_BASE->CINT = DMA_CINT_CINT(SPI_BRIDGE_DMA_RX_CHANNEL);
        LPSPI_RX_DMA_IRQHandler();
    }
}

status_t SPI_BridgeInit(void)
{
    memset((void *)last_rx, 0, sizeof(last_rx));
    memset((void *)last_tx, 0, sizeof(last_tx));

    build_pattern_frame(tx_buf[0]);
    build_pattern_frame(tx_buf[1]);

    dma_configure_lpspi_channels();
    lpspi_slave_init_with_dma();

    active_idx = 0U;
    completed_idx = 0U;
    completed_pending = false;

    arm_lpspi_dma(tx_buf[active_idx], rx_buf[active_idx]);

    return kStatus_Success;
}

void SPI_BridgeTask(void *param)
{
    (void)param;

    s_bridgeTaskHandle = xTaskGetCurrentTaskHandle();

    for (;;)
    {
        (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000U));

        uint8_t idx = 0U;
        if (completed_fetch(&idx))
        {
            rt1064_spi_bringup_process(idx);
        }
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
