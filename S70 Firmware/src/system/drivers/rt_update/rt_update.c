#include "rt_update.h"

#include <asf.h>
#include <string.h>

#include "Debugging.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "spi_bridge_protocol.h"
#include "user_spi.h"

bool load_rt_hex = false;

#define RT_UPDATE_PAGE_SIZE 256U
#define RT_UPDATE_SECTOR_SIZE 4096U

#define RT_UPDATE_ADDR_MIN 0x70000000UL
#define RT_UPDATE_ADDR_MAX 0x70400000UL

#define RT_UPDATE_LINE_MAX 520U
#define RT_UPDATE_RETRY_MAX 3U
#define RT_UPDATE_SPI_MAX_SIZE (RT_UPDATE_PAGE_SIZE + 16U)

typedef enum {
    RT_UPDATE_STATE_IDLE = 0,
    RT_UPDATE_STATE_STREAMING,
    RT_UPDATE_STATE_DONE,
    RT_UPDATE_STATE_ERROR,
} rt_update_state_t;

typedef struct {
    rt_update_state_t state;
    uint32_t upper16;
    uint32_t current_page_base;
    uint32_t current_sector_base;
    bool page_dirty;
    bool sector_valid;
    uint8_t page_buffer[RT_UPDATE_PAGE_SIZE];
    size_t line_length;
    char line_buffer[RT_UPDATE_LINE_MAX];
    bool saw_eof;
} rt_update_context_t;

static rt_update_context_t s_rt_update;

static uint8_t rt_hex_nibble(char value, bool* ok) {
    if ((value >= '0') && (value <= '9')) {
        return (uint8_t)(value - '0');
    }
    if ((value >= 'A') && (value <= 'F')) {
        return (uint8_t)(10 + (value - 'A'));
    }
    if ((value >= 'a') && (value <= 'f')) {
        return (uint8_t)(10 + (value - 'a'));
    }
    *ok = false;
    return 0;
}

static bool rt_hex_parse_byte(const char* src, uint8_t* out) {
    bool ok = true;
    uint8_t hi = rt_hex_nibble(src[0], &ok);
    uint8_t lo = rt_hex_nibble(src[1], &ok);
    if (!ok) {
        return false;
    }
    *out = (uint8_t)((hi << 4U) | lo);
    return true;
}

static bool rt_hex_parse_line(const char* line, size_t length, uint8_t* data,
                              size_t data_capacity, uint8_t* data_len,
                              uint16_t* offset, uint8_t* record_type) {
    if ((length < 11U) || (line[0] != ':')) {
        return false;
    }

    uint8_t byte_count = 0;
    uint8_t offset_hi = 0;
    uint8_t offset_lo = 0;
    uint8_t type = 0;

    if (!rt_hex_parse_byte(&line[1], &byte_count) ||
        !rt_hex_parse_byte(&line[3], &offset_hi) ||
        !rt_hex_parse_byte(&line[5], &offset_lo) ||
        !rt_hex_parse_byte(&line[7], &type)) {
        return false;
    }

    size_t expected = 11U + (size_t)byte_count * 2U;
    if (length < expected) {
        return false;
    }
    if (byte_count > data_capacity) {
        return false;
    }

    uint8_t checksum = 0;
    checksum = (uint8_t)(checksum + byte_count);
    checksum = (uint8_t)(checksum + offset_hi);
    checksum = (uint8_t)(checksum + offset_lo);
    checksum = (uint8_t)(checksum + type);

    const char* cursor = &line[9];
    for (uint8_t i = 0; i < byte_count; ++i) {
        uint8_t value = 0;
        if (!rt_hex_parse_byte(cursor, &value)) {
            return false;
        }
        data[i] = value;
        checksum = (uint8_t)(checksum + value);
        cursor += 2;
    }

    uint8_t expected_checksum = 0;
    if (!rt_hex_parse_byte(cursor, &expected_checksum)) {
        return false;
    }
    checksum = (uint8_t)(checksum + expected_checksum);
    if (checksum != 0) {
        return false;
    }

    *data_len = byte_count;
    *offset = (uint16_t)((offset_hi << 8U) | offset_lo);
    *record_type = type;
    return true;
}

static bool rt_update_address_valid(uint32_t address) {
    return (address >= RT_UPDATE_ADDR_MIN) && (address < RT_UPDATE_ADDR_MAX);
}

static void rt_update_fill_page(void) {
    (void)memset(s_rt_update.page_buffer, 0xFF,
                 sizeof(s_rt_update.page_buffer));
}

static uint32_t rt_update_align_down(uint32_t value, uint32_t align) {
    return value - (value % align);
}

static bool rt_update_spi_exchange(const uint8_t* tx, uint8_t* rx,
                                   uint16_t length) {
    if (length == 0U) {
        return true;
    }

    uint8_t buffer[RT_UPDATE_SPI_MAX_SIZE];
    if (length > sizeof(buffer)) {
        return false;
    }

    memcpy(buffer, tx, length);
    spi_status_t status = spi_transfer(_SPI_MODE_0, true, CS_NO_TOGGLE,
                                       CS_RT_UPDATE, buffer, length);
    if (status != SPI_OK) {
        return false;
    }

    if (rx != NULL) {
        memcpy(rx, buffer, length);
    }
    return true;
}

static bool rt_update_send_command(uint8_t command, const uint8_t* payload,
                                   uint16_t payload_length, uint8_t* response,
                                   uint16_t response_length) {
    uint8_t header[2];
    uint8_t tx[RT_UPDATE_SPI_MAX_SIZE];
    uint8_t rx[RT_UPDATE_SPI_MAX_SIZE];

    uint16_t status_index = (uint16_t)(payload_length + 2U);
    uint16_t extra_length = 0U;
    if (response_length > 1U) {
        extra_length = (uint16_t)(response_length - 1U);
    }

    uint16_t total = (uint16_t)(status_index + 1U + extra_length);
    if (total > sizeof(tx)) {
        return false;
    }

    header[0] = command;
    header[1] = (uint8_t)payload_length;

    tx[0] = header[0];
    tx[1] = header[1];
    if (payload_length > 0U) {
        (void)memcpy(&tx[2], payload, payload_length);
    }
    (void)memset(&tx[2U + payload_length], 0x00, total - (payload_length + 2U));

    if (xSPI_Semaphore != NULL) {
        xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
    }

    bool ok = rt_update_spi_exchange(tx, rx, total);

    if (xSPI_Semaphore != NULL) {
        xSemaphoreGive(xSPI_Semaphore);
    }

    if (!ok) {
        return false;
    }

    uint8_t status = rx[status_index];
    if ((response != NULL) && (response_length > 0U)) {
        response[0] = status;
        if (response_length > 1U) {
            memcpy(&response[1], &rx[status_index + 1U], response_length - 1U);
        }
    }

    return status == RT_UPDATE_STATUS_OK;
}

static bool rt_update_enter_update_mode(void) {
    for (uint8_t attempt = 0; attempt < RT_UPDATE_RETRY_MAX; ++attempt) {
        if (rt_update_send_command(RT_UPDATE_CMD_ENTER_UPDATE_MODE, NULL, 0U,
                                   NULL, 0U)) {
            return true;
        }
    }
    return false;
}

static bool rt_update_erase_sector(uint32_t base_address) {
    uint8_t payload[4];
    memcpy(payload, &base_address, sizeof(base_address));
    for (uint8_t attempt = 0; attempt < RT_UPDATE_RETRY_MAX; ++attempt) {
        if (rt_update_send_command(RT_UPDATE_CMD_ERASE_SECTOR, payload,
                                   sizeof(payload), NULL, 0U)) {
            return true;
        }
    }
    return false;
}

static bool rt_update_program_page(uint32_t base_address, const uint8_t* data) {
    uint8_t payload[4 + RT_UPDATE_PAGE_SIZE];
    memcpy(payload, &base_address, sizeof(base_address));
    memcpy(payload + 4, data, RT_UPDATE_PAGE_SIZE);

    for (uint8_t attempt = 0; attempt < RT_UPDATE_RETRY_MAX; ++attempt) {
        if (rt_update_send_command(RT_UPDATE_CMD_PROGRAM_PAGE, payload,
                                   sizeof(payload), NULL, 0U)) {
            return true;
        }
    }
    return false;
}

static bool rt_update_read_back(uint32_t base_address, uint8_t* buffer) {
    uint8_t payload[6];
    uint16_t length = RT_UPDATE_PAGE_SIZE;
    memcpy(payload, &base_address, sizeof(base_address));
    memcpy(payload + 4, &length, sizeof(length));

    uint8_t response[1 + RT_UPDATE_PAGE_SIZE];
    for (uint8_t attempt = 0; attempt < RT_UPDATE_RETRY_MAX; ++attempt) {
        if (!rt_update_send_command(RT_UPDATE_CMD_READ_BACK, payload,
                                    sizeof(payload), response,
                                    sizeof(response))) {
            continue;
        }
        memcpy(buffer, &response[1], RT_UPDATE_PAGE_SIZE);
        return true;
    }
    return false;
}

static bool rt_update_flush_page(USB_Slave_Message* slave_message) {
    if (!s_rt_update.page_dirty) {
        return true;
    }

    if (!rt_update_address_valid(s_rt_update.current_page_base)) {
        return false;
    }

    uint32_t sector_base = rt_update_align_down(s_rt_update.current_page_base,
                                                RT_UPDATE_SECTOR_SIZE);
    if ((!s_rt_update.sector_valid) ||
        (sector_base != s_rt_update.current_sector_base)) {
        if (!rt_update_erase_sector(sector_base)) {
            return false;
        }
        s_rt_update.current_sector_base = sector_base;
        s_rt_update.sector_valid = true;
    }

    if (!rt_update_program_page(s_rt_update.current_page_base,
                                s_rt_update.page_buffer)) {
        return false;
    }

    uint8_t verify[RT_UPDATE_PAGE_SIZE];
    if (!rt_update_read_back(s_rt_update.current_page_base, verify)) {
        return false;
    }

    if (memcmp(verify, s_rt_update.page_buffer, RT_UPDATE_PAGE_SIZE) != 0) {
        debug_print("RT update verify mismatch at 0x%08lx\r\n",
                    (unsigned long)s_rt_update.current_page_base);
        return false;
    }

    s_rt_update.page_dirty = false;
    (void)slave_message;
    return true;
}

static bool rt_update_handle_data(uint32_t address, const uint8_t* data,
                                  uint8_t length,
                                  USB_Slave_Message* slave_message) {
    if (!rt_update_address_valid(address)) {
        return false;
    }

    uint32_t page_base = rt_update_align_down(address, RT_UPDATE_PAGE_SIZE);
    if (page_base != s_rt_update.current_page_base) {
        if (!rt_update_flush_page(slave_message)) {
            return false;
        }

        s_rt_update.current_page_base = page_base;
        rt_update_fill_page();
    }

    uint32_t offset = address - page_base;
    if ((offset + length) > RT_UPDATE_PAGE_SIZE) {
        return false;
    }

    memcpy(&s_rt_update.page_buffer[offset], data, length);
    s_rt_update.page_dirty = true;
    return true;
}

static void rt_update_reset_state(void) {
    (void)memset(&s_rt_update, 0, sizeof(s_rt_update));
    rt_update_fill_page();
}

void rt_firmware_update_start(USB_Slave_Message* slave_message) {
    (void)slave_message;
    rt_update_reset_state();

    if (!rt_update_enter_update_mode()) {
        s_rt_update.state = RT_UPDATE_STATE_ERROR;
        load_rt_hex = false;
        debug_print("RT update: enter mode failed\r\n");
        return;
    }

    s_rt_update.state = RT_UPDATE_STATE_STREAMING;
    load_rt_hex = true;
}

void rt_firmware_update_abort(USB_Slave_Message* slave_message) {
    (void)slave_message;
    load_rt_hex = false;
    rt_update_reset_state();
    s_rt_update.state = RT_UPDATE_STATE_ERROR;
}

static bool rt_update_finalize(USB_Slave_Message* slave_message) {
    if (!rt_update_flush_page(slave_message)) {
        return false;
    }

    if (!rt_update_send_command(RT_UPDATE_CMD_FINALIZE_AND_REBOOT, NULL, 0U,
                                NULL, 0U)) {
        return false;
    }

    return true;
}

static bool rt_update_process_line(const char* line, size_t length,
                                   USB_Slave_Message* slave_message) {
    uint8_t data[RT_UPDATE_PAGE_SIZE];
    uint8_t data_len = 0;
    uint16_t offset = 0;
    uint8_t record_type = 0;

    if (!rt_hex_parse_line(line, length, data, sizeof(data), &data_len, &offset,
                           &record_type)) {
        debug_print("RT update: bad HEX line\r\n");
        return false;
    }

    switch (record_type) {
        case 0x00: {
            uint32_t address = (s_rt_update.upper16 << 16U) | offset;
            return rt_update_handle_data(address, data, data_len,
                                         slave_message);
        }
        case 0x01:
            s_rt_update.saw_eof = true;
            return true;
        case 0x04:
            if (data_len != 2U) {
                return false;
            }
            s_rt_update.upper16 = ((uint32_t)data[0] << 8U) | data[1];
            return true;
        default:
            return false;
    }
}

size_t rt_firmware_update_consume(const uint8_t* buffer, size_t length,
                                  USB_Slave_Message* slave_message) {
    if (s_rt_update.state != RT_UPDATE_STATE_STREAMING) {
        return length;
    }

    size_t consumed = 0;
    for (; consumed < length; ++consumed) {
        uint8_t byte = buffer[consumed];
        if ((byte == '\r') || (byte == '\n')) {
            if (s_rt_update.line_length > 0U) {
                s_rt_update.line_buffer[s_rt_update.line_length] = '\0';
                if (!rt_update_process_line(s_rt_update.line_buffer,
                                            s_rt_update.line_length,
                                            slave_message)) {
                    s_rt_update.state = RT_UPDATE_STATE_ERROR;
                    load_rt_hex = false;
                    return consumed + 1;
                }
                s_rt_update.line_length = 0;
            }

            if (s_rt_update.saw_eof) {
                if (!rt_update_finalize(slave_message)) {
                    s_rt_update.state = RT_UPDATE_STATE_ERROR;
                } else {
                    s_rt_update.state = RT_UPDATE_STATE_DONE;
                }
                load_rt_hex = false;
                return consumed + 1;
            }
            continue;
        }

        if (s_rt_update.line_length + 1U >= sizeof(s_rt_update.line_buffer)) {
            s_rt_update.state = RT_UPDATE_STATE_ERROR;
            load_rt_hex = false;
            return consumed + 1;
        }
        s_rt_update.line_buffer[s_rt_update.line_length++] = (char)byte;
    }

    return consumed;
}
