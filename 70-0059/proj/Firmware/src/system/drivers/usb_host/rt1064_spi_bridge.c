// rt1064_spi_bridge.c

#include "rt1064_spi_bridge.h"

#include <string.h>

#include "Debugging.h"
#include "FreeRTOS.h"
#include "sys_task.h"
#include "task.h"
#include "user_spi.h"

#define SPI_BRIDGE_MAX_PAYLOAD_LENGTH (63U)
#define SPI_BRIDGE_FRAME_SOF (0xA5U)
#define SPI_BRIDGE_FRAME_HEADER_SIZE (4U)
#define SPI_BRIDGE_FRAME_CRC_SIZE (2U)
#define SPI_BRIDGE_FRAME_SIZE \
    (SPI_BRIDGE_FRAME_HEADER_SIZE + SPI_BRIDGE_MAX_PAYLOAD_LENGTH + SPI_BRIDGE_FRAME_CRC_SIZE)

#define SPI_BRIDGE_TYPE_PING (0x01U)
#define SPI_BRIDGE_TYPE_ECHO (0x02U)
#define SPI_BRIDGE_TYPE_RESPONSE (0x80U)
#define SPI_BRIDGE_TYPE_NOT_READY (0x7FU)
#define SPI_BRIDGE_TYPE_ERROR (0x7EU)
#define SPI_BRIDGE_CRC_POLY (0x1021U)
#define SPI_BRIDGE_CRC_INIT (0xFFFFU)

#define SPI_BRIDGE_POLL_PERIOD_MS (10U)

static uint16_t spi_bridge_crc_update(uint16_t crc, uint8_t data) {
    crc ^= (uint16_t)data << 8U;
    for (uint8_t i = 0U; i < 8U; ++i) {
        if ((crc & 0x8000U) != 0U) {
            crc = (uint16_t)((crc << 1U) ^ SPI_BRIDGE_CRC_POLY);
        } else {
            crc <<= 1U;
        }
    }
    return crc;
}


static uint16_t spi_bridge_compute_frame_crc(const uint8_t* frame) {
    uint8_t length = frame[3];
    uint16_t crc = SPI_BRIDGE_CRC_INIT;

    if (length > SPI_BRIDGE_MAX_PAYLOAD_LENGTH) {
        return 0U;
    }

    for (uint8_t i = 0U; i < (SPI_BRIDGE_FRAME_HEADER_SIZE + length); ++i) {
        crc = spi_bridge_crc_update(crc, frame[i]);
    }

    return crc;
}

static void spi_bridge_build_frame(uint8_t* frame, uint8_t seq, uint8_t type,
                                   const uint8_t* payload, uint8_t length) {
    if (frame == NULL) {
        return;
    }

    if (length > SPI_BRIDGE_MAX_PAYLOAD_LENGTH) {
        length = SPI_BRIDGE_MAX_PAYLOAD_LENGTH;
    }

    frame[0] = SPI_BRIDGE_FRAME_SOF;
    frame[1] = seq;
    frame[2] = type;
    frame[3] = length;

    if ((payload != NULL) && (length > 0U)) {
        (void)memcpy(&frame[SPI_BRIDGE_FRAME_HEADER_SIZE], payload, length);
    }
    if (length < SPI_BRIDGE_MAX_PAYLOAD_LENGTH) {
        (void)memset(&frame[SPI_BRIDGE_FRAME_HEADER_SIZE + length], 0,
                     SPI_BRIDGE_MAX_PAYLOAD_LENGTH - length);
    }

    uint16_t crc = spi_bridge_compute_frame_crc(frame);
    frame[SPI_BRIDGE_FRAME_SIZE - 2U] = (uint8_t)(crc & 0xFFU);
    frame[SPI_BRIDGE_FRAME_SIZE - 1U] = (uint8_t)((crc >> 8U) & 0xFFU);
}

static bool spi_bridge_frame_valid(const uint8_t* frame) {
    if ((frame == NULL) || (frame[0] != SPI_BRIDGE_FRAME_SOF)) {
        return false;
    }

    uint8_t length = frame[3];
    if (length > SPI_BRIDGE_MAX_PAYLOAD_LENGTH) {
        return false;
    }

    uint16_t expected = spi_bridge_compute_frame_crc(frame);
    uint16_t received = (uint16_t)frame[SPI_BRIDGE_FRAME_SIZE - 2U] |
                        ((uint16_t)frame[SPI_BRIDGE_FRAME_SIZE - 1U] << 8U);

    return (expected != 0U) && (expected == received);
}

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

static bool spi_bridge_transfer_frame(const uint8_t* tx_frame,
                                      uint8_t* rx_frame) {
    if ((tx_frame == NULL) || (rx_frame == NULL)) {
        return false;
    }

    if (!spi_bridge_start_transaction()) {
        return false;
    }

    for (uint8_t i = 0U; i < SPI_BRIDGE_FRAME_SIZE; ++i) {
        uint8_t value = tx_frame[i];
        if (spi_partial_transfer(&value) != SPI_OK) {
            spi_bridge_end_transaction();
            return false;
        }
        rx_frame[i] = value;
    }

    spi_bridge_end_transaction();
    return true;
}

static void task_spi_bridge(void* pvParameters) {
    /* Set the PCK0 clock source to MAIN_CLK and clock divider to 0*/
    ((Pmc*)PMC)->PMC_PCK[0] = 1;

    /* Enable PCK0 output to pin*/
    ((Pmc*)PMC)->PMC_SCER |= PMC_SCER_PCK0;

    /* Setup the clock pin from the processor to CPLD to mode MUX D*/
    ((Pio*)PIOB)->PIO_ABCDSR[0] |= PIO_PB12;
    ((Pio*)PIOB)->PIO_ABCDSR[1] |= PIO_PB12;
    /*Disables the PIO from controlling the corresponding pin (enables
     * peripheral control of the pin).*/
    ((Pio*)PIOB)->PIO_PDR = PIO_PB12;

    (void)pvParameters;

    uint8_t seq = 0U;
    uint8_t last_seq = 0U;
    uint8_t last_type = 0U;
    uint8_t last_length = 0U;
    uint8_t last_payload[SPI_BRIDGE_MAX_PAYLOAD_LENGTH] = {0U};

    for (;;) {
        uint8_t tx_frame[SPI_BRIDGE_FRAME_SIZE];
        uint8_t rx_frame[SPI_BRIDGE_FRAME_SIZE];
        uint8_t payload[SPI_BRIDGE_MAX_PAYLOAD_LENGTH];
        uint8_t length = 0U;
        uint8_t type = ((seq & 0x01U) != 0U) ? SPI_BRIDGE_TYPE_ECHO : SPI_BRIDGE_TYPE_PING;

        if (type == SPI_BRIDGE_TYPE_ECHO) {
            length = 4U;
            payload[0] = seq;
            payload[1] = (uint8_t)(seq + 1U);
            payload[2] = (uint8_t)(seq + 2U);
            payload[3] = (uint8_t)(seq + 3U);
        }

        spi_bridge_build_frame(tx_frame, seq, type, payload, length);

        if (spi_bridge_transfer_frame(tx_frame, rx_frame)) {
            if (seq > 0U) {
                if (!spi_bridge_frame_valid(rx_frame)) {
                    debug_print("SPI bridge: invalid response frame\r\n");
                } else {
                    uint8_t resp_seq = rx_frame[1];
                    uint8_t resp_type = rx_frame[2];
                    uint8_t resp_length = rx_frame[3];
                    const uint8_t *resp_payload = &rx_frame[SPI_BRIDGE_FRAME_HEADER_SIZE];

                    if (resp_type == SPI_BRIDGE_TYPE_NOT_READY) {
                        debug_print("SPI bridge: not ready response (seq=%u)\r\n", resp_seq);
                    } else if (resp_type == SPI_BRIDGE_TYPE_ERROR) {
                        debug_print("SPI bridge: error response (code=%u)\r\n", resp_payload[0]);
                    } else {
                        if (resp_seq != last_seq) {
                            debug_print("SPI bridge: seq mismatch (got=%u expected=%u)\r\n",
                                        resp_seq, last_seq);
                        }
                        if (resp_type != (uint8_t)(SPI_BRIDGE_TYPE_RESPONSE | last_type)) {
                            debug_print("SPI bridge: type mismatch (got=0x%02X expected=0x%02X)\r\n",
                                        resp_type, (uint8_t)(SPI_BRIDGE_TYPE_RESPONSE | last_type));
                        }
                        if (last_type == SPI_BRIDGE_TYPE_ECHO) {
                            if ((resp_length != last_length) ||
                                (memcmp(resp_payload, last_payload, last_length) != 0)) {
                                debug_print("SPI bridge: echo payload mismatch\r\n");
                            }
                        }
                        debug_print("SPI bridge: response seq=%u type=0x%02X len=%u\r\n",
                                    resp_seq, resp_type, resp_length);
                    }
                }
            }
        } else {
            debug_print("SPI bridge: transfer failed\r\n");
        }

        last_seq = seq;
        last_type = type;
        last_length = length;
        if (length > 0U) {
            (void)memcpy(last_payload, payload, length);
        }

        ++seq;
        vTaskDelay(pdMS_TO_TICKS(SPI_BRIDGE_POLL_PERIOD_MS));
    }
}

void rt1064_spi_bridge_init(void) {
    if (xTaskCreate(task_spi_bridge, "SPI Bridge", TASK_SPI_BRIDGE_STACK_SIZE,
                    NULL, TASK_SPI_BRIDGE_STACK_PRIORITY, NULL) != pdPASS) {
        debug_print("Failed to create SPI bridge task\r\n");
    }
}
