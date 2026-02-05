/*
 * SPI bring-up test (RT1064 slave side)
 *
 * Fixed 64-byte full-duplex frames with CRC16/CCITT-FALSE and double-buffered
 * TX/RX data ownership for DMA-style handoff.
 */
#include "spi_bridge.h"

#include <string.h>

#include "FreeRTOS.h"
#include "fsl_clock.h"
#include "task.h"

#define FRAME_SIZE (64U)
#define FRAME_DATA_SIZE (62U)

#define SPI_BRIDGE_DMA_BASE DMA0
#define SPI_BRIDGE_DMAMUX_BASE DMAMUX
#define SPI_BRIDGE_DMA_RX_CHANNEL (0U)
#define SPI_BRIDGE_DMA_TX_CHANNEL (1U)
#define SPI_BRIDGE_DMA_RX_REQUEST (13U) /* kDmaRequestMuxLPSPI1Rx in PERI_DMAMUX.h */
#define SPI_BRIDGE_DMA_TX_REQUEST (14U) /* kDmaRequestMuxLPSPI1Tx in PERI_DMAMUX.h */

#ifndef SPI_BRIDGE_SPI_BASE
#define SPI_BRIDGE_SPI_BASE LPSPI1
#endif

volatile bool last_rx_good = false;
volatile uint32_t good_count = 0U;
volatile uint32_t bad_count = 0U;

uint8_t last_rx[FRAME_SIZE];
uint8_t last_tx[FRAME_SIZE];

static uint8_t tx_buf[2][FRAME_SIZE];
static uint8_t rx_buf[2][FRAME_SIZE];
static volatile uint8_t active_idx = 0U;
static volatile bool completed_pending = false;
static volatile uint8_t completed_idx = 0U;

static TaskHandle_t s_bridgeTaskHandle;

static void dma_configure_lpspi_channels(void)
{
    CLOCK_EnableClock(kCLOCK_Dma);
    CLOCK_EnableClock(kCLOCK_Dmamux);

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

static uint16_t crc16_ccitt_false(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xFFFFU;

    for (uint32_t i = 0U; i < len; i++)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t bit = 0U; bit < 8U; bit++)
        {
            crc = (crc & 0x8000U) != 0U ? (uint16_t)((crc << 1) ^ 0x1021U) : (uint16_t)(crc << 1);
        }
    }

    return crc;
}

static void frame_write_crc(uint8_t frame[FRAME_SIZE])
{
    uint16_t crc = crc16_ccitt_false(frame, FRAME_DATA_SIZE);
    frame[62] = (uint8_t)(crc & 0xFFU);
    frame[63] = (uint8_t)(crc >> 8);
}

static bool frame_check_crc(const uint8_t frame[FRAME_SIZE])
{
    uint16_t calc = crc16_ccitt_false(frame, FRAME_DATA_SIZE);
    uint16_t recv = (uint16_t)frame[62] | ((uint16_t)frame[63] << 8);
    return calc == recv;
}

static void build_test_frame(uint8_t frame[FRAME_SIZE], uint8_t start)
{
    frame[0] = 0xA5U;
    frame[1] = 0x12U;
    frame[2] = 0x34U;

    uint8_t value = start;
    for (uint32_t i = 3U; i <= 61U; i++)
    {
        frame[i] = value;
        value++;
    }

    frame_write_crc(frame);
}

static void lpspi_slave_init_with_dma(void)
{
    CLOCK_SetMux(kCLOCK_LpspiMux, 1U);
    CLOCK_SetDiv(kCLOCK_LpspiDiv, 7U);
    CLOCK_EnableClock(kCLOCK_Lpspi1);

    SPI_BRIDGE_SPI_BASE->CR = LPSPI_CR_RST_MASK;
    SPI_BRIDGE_SPI_BASE->CR = 0U;
    SPI_BRIDGE_SPI_BASE->CR = LPSPI_CR_RRF_MASK | LPSPI_CR_RTF_MASK;
    SPI_BRIDGE_SPI_BASE->CFGR1 = LPSPI_CFGR1_MASTER(0U) | LPSPI_CFGR1_NOSTALL(1U) | LPSPI_CFGR1_PINCFG(0U);
    SPI_BRIDGE_SPI_BASE->FCR = LPSPI_FCR_TXWATER(0U) | LPSPI_FCR_RXWATER(0U);
    SPI_BRIDGE_SPI_BASE->TCR = LPSPI_TCR_FRAMESZ(7U) | LPSPI_TCR_PCS(0U) | LPSPI_TCR_CPHA(0U) | LPSPI_TCR_CPOL(0U);
    SPI_BRIDGE_SPI_BASE->SR = LPSPI_SR_WCF_MASK | LPSPI_SR_FCF_MASK | LPSPI_SR_TCF_MASK | LPSPI_SR_TEF_MASK |
                              LPSPI_SR_REF_MASK | LPSPI_SR_DMF_MASK;

    /* DMA hookups are project-specific; these requests enable peripheral DMA. */
    SPI_BRIDGE_SPI_BASE->DER = LPSPI_DER_TDDE_MASK | LPSPI_DER_RDDE_MASK;
    SPI_BRIDGE_SPI_BASE->CR = LPSPI_CR_MEN_MASK;
}

static void arm_lpspi_dma(uint8_t *tx, uint8_t *rx)
{
    SPI_BRIDGE_DMA_BASE->CERQ = DMA_CERQ_CERQ(SPI_BRIDGE_DMA_RX_CHANNEL);
    SPI_BRIDGE_DMA_BASE->CERQ = DMA_CERQ_CERQ(SPI_BRIDGE_DMA_TX_CHANNEL);

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
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_TX_CHANNEL].CITER_ELINKNO = DMA_CITER_ELINKNO_CITER(FRAME_SIZE);
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_TX_CHANNEL].DLAST_SGA = 0;
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_TX_CHANNEL].CSR = DMA_CSR_DREQ_MASK;
    SPI_BRIDGE_DMA_BASE->TCD[SPI_BRIDGE_DMA_TX_CHANNEL].BITER_ELINKNO = DMA_BITER_ELINKNO_BITER(FRAME_SIZE);

    SPI_BRIDGE_DMA_BASE->CINT = DMA_CINT_CINT(SPI_BRIDGE_DMA_RX_CHANNEL);
    SPI_BRIDGE_DMA_BASE->CINT = DMA_CINT_CINT(SPI_BRIDGE_DMA_TX_CHANNEL);
    SPI_BRIDGE_DMA_BASE->CDNE = DMA_CDNE_CDNE(SPI_BRIDGE_DMA_RX_CHANNEL);
    SPI_BRIDGE_DMA_BASE->CDNE = DMA_CDNE_CDNE(SPI_BRIDGE_DMA_TX_CHANNEL);

    SPI_BRIDGE_DMA_BASE->SERQ = DMA_SERQ_SERQ(SPI_BRIDGE_DMA_TX_CHANNEL);
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
    uint8_t prep_idx = idx;

    memcpy(last_rx, rx_buf[idx], FRAME_SIZE);
    last_rx_good = frame_check_crc(last_rx);

    if (last_rx_good)
    {
        good_count++;
    }
    else
    {
        bad_count++;
    }

    build_test_frame(tx_buf[prep_idx], 0x80U);
    memcpy(last_tx, tx_buf[prep_idx], FRAME_SIZE);
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
    memset(last_rx, 0, sizeof(last_rx));
    memset(last_tx, 0, sizeof(last_tx));

    build_test_frame(tx_buf[0], 0x80U);
    build_test_frame(tx_buf[1], 0x80U);

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
        while (completed_fetch(&idx))
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
