// rt1064_spi_bridge.c

#include "rt1064_spi_bridge.h"

#include "Debugging.h"
#include "FreeRTOS.h"
#include "sys_task.h"
#include "task.h"
#include "user_spi.h"

#define SPI_BRIDGE_POLL_PERIOD_MS (100U)

static bool spi_bridge_start_transaction(void) {
    xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
    if (spi_start_transfer(_SPI_MODE_0, CS_NO_TOGGLE, CS_RT1064) != SPI_OK) {
        xSemaphoreGive(xSPI_Semaphore);
        return false;
    }
    return true;
}

static void spi_bridge_end_transaction(void) {
    (void)spi_end_transfer();
    xSemaphoreGive(xSPI_Semaphore);
}

static bool spi_bridge_exchange_byte(uint8_t tx_byte, uint8_t *rx_byte) {
    uint8_t value = tx_byte;
    if (spi_partial_transfer(&value) != SPI_OK) {
        return false;
    }
    if (rx_byte != NULL) {
        *rx_byte = value;
    }
    return true;
}

static void task_spi_bridge(void *pvParameters) {
    /* Set the PCK0 clock source to MAIN_CLK and clock divider to 0 */
    ((Pmc *)PMC)->PMC_PCK[0] = 1;

    /* Enable PCK0 output to pin */
    ((Pmc *)PMC)->PMC_SCER |= PMC_SCER_PCK0;

    /* Setup the clock pin from the processor to CPLD to mode MUX D */
    ((Pio *)PIOB)->PIO_ABCDSR[0] |= PIO_PB12;
    ((Pio *)PIOB)->PIO_ABCDSR[1] |= PIO_PB12;
    /* Disables the PIO from controlling the corresponding pin (enables
     * peripheral control of the pin). */
    ((Pio *)PIOB)->PIO_PDR = PIO_PB12;

    (void)pvParameters;

    uint8_t tx_byte = 0x00U;

    for (;;) {
        if (spi_bridge_start_transaction()) {
            uint8_t stale_rx = 0x00U;
            uint8_t response = 0x00U;

            if (!spi_bridge_exchange_byte(tx_byte, &stale_rx)) {
                debug_print("SPI bridge: transfer failed (tx)\r\n");
                spi_bridge_end_transaction();
                vTaskDelay(pdMS_TO_TICKS(SPI_BRIDGE_POLL_PERIOD_MS));
                continue;
            }

            if (!spi_bridge_exchange_byte(0x00U, &response)) {
                debug_print("SPI bridge: transfer failed (rx)\r\n");
                spi_bridge_end_transaction();
                vTaskDelay(pdMS_TO_TICKS(SPI_BRIDGE_POLL_PERIOD_MS));
                continue;
            }

            spi_bridge_end_transaction();
            debug_print("SPI bridge: sent 0x%02X received 0x%02X\r\n", tx_byte, response);
            ++tx_byte;
        } else {
            debug_print("SPI bridge: failed to start transaction\r\n");
        }

        vTaskDelay(pdMS_TO_TICKS(SPI_BRIDGE_POLL_PERIOD_MS));
    }
}

void rt1064_spi_bridge_init(void) {
    if (xTaskCreate(task_spi_bridge, "SPI Bridge", TASK_SPI_BRIDGE_STACK_SIZE,
                    NULL, TASK_SPI_BRIDGE_STACK_PRIORITY, NULL) != pdPASS) {
        debug_print("Failed to create SPI bridge task\r\n");
    }
}
