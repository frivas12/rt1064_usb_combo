// rt1064_spi_bridge.c

#include "rt1064_spi_bridge.h"

#include <string.h>

#include "Debugging.h"
#include "FreeRTOS.h"
#include "sys_task.h"
#include "task.h"
#include "user_spi.h"

#define SPI_BRIDGE_POLL_PERIOD_MS (10U)

volatile bool last_rx_good = false;
volatile uint32_t good_count = 0U;
volatile uint32_t bad_count = 0U;

volatile uint8_t last_rx[SPI_BRIDGE_FRAME_SIZE];
volatile uint8_t last_tx[SPI_BRIDGE_FRAME_SIZE];

volatile uint8_t spi_active_idx = 0U;
volatile uint8_t spi_last_completed_idx = 0U;

static uint8_t s_tx_frame[SPI_BRIDGE_FRAME_SIZE];
static uint8_t s_rx_frame[SPI_BRIDGE_FRAME_SIZE];

static uint16_t crc16_ccitt_false(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xFFFFU;

    for (uint32_t i = 0U; i < len; i++)
    {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t b = 0U; b < 8U; b++)
        {
            if ((crc & 0x8000U) != 0U)
            {
                crc = (uint16_t)((crc << 1) ^ 0x1021U);
            }
            else
            {
                crc = (uint16_t)(crc << 1);
            }
        }
    }

    return crc;
}

static void frame_write_crc(uint8_t frame[SPI_BRIDGE_FRAME_SIZE])
{
    uint16_t crc = crc16_ccitt_false(frame, SPI_BRIDGE_DATA_BYTES);
    frame[62] = (uint8_t)(crc & 0xFFU);
    frame[63] = (uint8_t)(crc >> 8);
}

static bool frame_check_crc(const uint8_t frame[SPI_BRIDGE_FRAME_SIZE])
{
    uint16_t calc = crc16_ccitt_false(frame, SPI_BRIDGE_DATA_BYTES);
    uint16_t recv = (uint16_t)frame[62] | ((uint16_t)frame[63] << 8);
    return calc == recv;
}

static void build_test_frame(uint8_t frame[SPI_BRIDGE_FRAME_SIZE], uint8_t start)
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

static bool spi_bridge_start_transaction(void)
{
    return (spi_start_transfer(_SPI_MODE_0, CS_NO_TOGGLE, CS_RT1064) == SPI_OK);
}

static void spi_bridge_end_transaction(void)
{
    (void)spi_end_transfer();
}

static bool spi_transfer_64(const uint8_t *tx, uint8_t *rx)
{
    for (uint32_t i = 0U; i < SPI_BRIDGE_FRAME_SIZE; i++)
    {
        uint8_t value = tx[i];
        if (spi_partial_transfer(&value) != SPI_OK)
        {
            return false;
        }
        rx[i] = value;
    }

    return true;
}

static bool spi_transfer_64_with_optional_retry(const uint8_t *tx, uint8_t *rx)
{
    if (!spi_bridge_start_transaction())
    {
        return false;
    }

    bool ok = spi_transfer_64(tx, rx);
    spi_bridge_end_transaction();
    if (!ok)
    {
        return false;
    }

    if (rx[0] == 0xA5U)
    {
        return true;
    }

    /*
     * On RT1064 LPSPI slave + DMA bring-up the first frame after reset can be
     * invalid if MISO is not driven on the earliest clock edges. Retry once.
     */
    if (!spi_bridge_start_transaction())
    {
        return false;
    }

    ok = spi_transfer_64(tx, rx);
    spi_bridge_end_transaction();
    return ok;
}

static void task_spi_bridge(void *pvParameters)
{
    (void)pvParameters;

    uint8_t start = 0x00U;

    for (;;)
    {
        spi_active_idx ^= 1U;
        build_test_frame(s_tx_frame, start);
        start++;
        memcpy((void *)last_tx, s_tx_frame, SPI_BRIDGE_FRAME_SIZE);

        xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
        {
            bool transfer_ok = spi_transfer_64_with_optional_retry(s_tx_frame, s_rx_frame);
            xSemaphoreGive(xSPI_Semaphore);

            if (transfer_ok)
            {
                spi_last_completed_idx = spi_active_idx;
                memcpy((void *)last_rx, s_rx_frame, SPI_BRIDGE_FRAME_SIZE);
                last_rx_good = frame_check_crc(s_rx_frame);
                if (last_rx_good)
                {
                    good_count++;
                }
                else
                {
                    bad_count++;
                }
            }
            else
            {
                bad_count++;
                last_rx_good = false;
                debug_print("SPI bridge: 64-byte transfer failed\r\n");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(SPI_BRIDGE_POLL_PERIOD_MS));
    }
}

void rt1064_spi_bridge_init(void)
{
    memset((void *)last_rx, 0, sizeof(last_rx));
    memset((void *)last_tx, 0, sizeof(last_tx));

    if (xTaskCreate(task_spi_bridge, "SPI Bridge", TASK_SPI_BRIDGE_STACK_SIZE, NULL, TASK_SPI_BRIDGE_STACK_PRIORITY,
                    NULL) != pdPASS)
    {
        debug_print("Failed to create SPI bridge task\r\n");
    }
}
