/**
 * @file device_detect.c
 *
 * @brief Functions for ???
 *
 */

#include <asf.h>
#include "FreeRTOSConfig.h"
#include "slots.h"
#include "device_detect.h"
#include "one_wire.h"
#include <cpld.h>
#include "delay.h"
#include "Debugging.h"
#include "user_usart.h"
#include "FreeRTOS.h"
#include "sys_task.h"
#include "string.h"
#include "supervisor.h"

#include "defs.hh"
#include "heartbeat_watchdog.hh"
#include "pnp_status.hh"

/****************************************************************************
 * Private Data
 ****************************************************************************/

#define DD_UART_READ_TRIES                                      10

// The amount of time to hold UART0 high to give enough power to write the EEPROM.
// Source: Programming Time, t_{PROG} (https://datasheets.maximintegrated.com/en/ds/DS28E07.pdf)
#define DEVICE_DETECT_COPY_SCRATCHPAD_DELAY_TIME                pdMS_TO_TICKS(12)

#define DEVICE_EEPROM_ADDRESS_VERSION                           0
#define DEVICE_EEPROM_ADDRESS_CHECKSUM                          1

#define DEVICE_EEPROM_ADDRESS_V1_HEADER                         2
#define DEVICE_EEPROM_ADDRESS_V1_DATA                           25

#define OW_EEPROM_SIZE                                  128
#define SCRATCHPAD_SIZE                                 8

constexpr pnp_status_t PNP_STATUS_MASK = ~(
    pnp_status::NO_DEVICE_CONNECTED |
    pnp_status::GENERAL_OW_ERROR |
    pnp_status::UNKNOWN_OW_VERSION | 
    pnp_status::OW_CORRUPTION |
    pnp_status::SERIAL_NUM_MISMATCH | 
    pnp_status::SIGNATURE_NOT_ALLOWED);

typedef struct __attribute__((packed)) {
    uint8_t version;
    uint8_t checksum;
} _device_eeprom_header_t;

typedef enum __attribute__((packed)){
    // Version Independent
    _DDC_RESET_FSM = 0,
    /**
     * Reset the FSM back to _DDS_NO_DEVICE.
     * Parameter 1:     Unused
     * Parameter 2:     Unused
     */

    _DDC_ENABLE_PROGRAMING,
    /**
     * Set the FSM into programming mode.
     * Parameter 1:     Unused
     * Parameter 2:     Unused
     */

    _DDC_STOP_PROGRAMMING,
    /**
     * Exit programming mode.
     * Parameter 1:     Unused
     * Parameter 2:     Unused
     */

    _DDC_CHECK_PROGRAMMING,
    /**
     * Checks the programmign mode.
     * Parameter 1:     Unused
     * Parameter 2:     Unused
     */

    _DDC_PROGRAM,
    /**
     * Program data onto one-wire.
     * Parameter 1:     Pointer to data to write to one-wire.
     * Parameter 2:     Amount of data stored in the buffer.
     */
} _device_detect_operation_t;

typedef struct __attribute__((packed)){
    _device_detect_operation_t operation; //One-byte long
    uint16_t parameter2;
    uint8_t _RESERVED; // Added padding bytes to dword-align parameter 1.
    union{
        void * ptr;
        uint32_t value;
    } parameter1;
    void *p_callback; // Callback when command has been handled.
    void *p_callback_param; // Extra bytes for a callback parameter.
} _device_detect_command_t;

constexpr uint64_t NO_DEVICE_SERIAL_NUMBER = 0xFFFFFFFFFFFFFFFF;

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static bool service_ow(uint8_t slot);
static bool reset_send(uint8_t slot);
static uint8_t reset_response(uint8_t slot);
static void write_srcatch_pad(uint8_t slot);
static bool read_scratch_pad(uint8_t slot);
static void copy_srcatch_pad(uint8_t slot);
static void read_memory_device(uint8_t slot);
static bool read_device(uint8_t slot);

static void task_device_detect(void * p_params);

/**
 * 
 * \param[in]       p_slot Slot pointer to read config from.
 * \param[out]      p_is_static Pointer to buffer that stores if device detection should be ignored.
 */
static void get_volatile_slot_config(Slots * const p_slot, bool * const p_is_static);

// Device-Detect helper functions
/**
 * Handles the device-detect FSM for all devices with version 1 of device-detect.
 * \param[in]       slot            Slot card being processed.
 * \param[inout]    p_device        Pointer to device.
 * \param[in]       device_detected If the device is present or not.
 * \param[in]       p_header        Header info from the device's eeprom.
 * \return Any PnP status flags to be OR-ed into the slot's status.
 */
static pnp_status_t handle_fsm_v1 (const slot_nums slot, Device * const p_device,
        bool device_detected, const _device_eeprom_header_t * const p_header);

static void set_ow_serial_number_raw(const slot_nums slot, const uint64_t serial_number)
{
    xSemaphoreTake(slots[slot].xSlot_Mutex, portMAX_DELAY);
    slots[slot].device.info.serial_num = serial_number;
    xSemaphoreGive(slots[slot].xSlot_Mutex);
}

/**
 * Sets the serial number for the device on a given slot card.
 * Only the lower 48-bits of the serial_number parameter will be saved.
 * \param[in]       slot The slot number to save the serial number to.
 * \param[in]       serial_number The 48-bit serial number to save.
 */
static inline void set_ow_serial_number(const slot_nums slot, uint64_t serial_number)
{
    const uint64_t MASK = 0x0000FFFFFFFFFFFF;
    serial_number &= MASK;
    set_ow_serial_number_raw(slot, serial_number);
}

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

#define DDEnableGates                           1

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/**
 * @brief Send reset we will wait for response on next service.
 * @param slot
 * @return If card is in slot return 1, else return 0
 */
static bool reset_send(uint8_t slot)
{
    static const uint8_t PRES = OW_PRESENCE;
    /*Check we have a card installed in slot, if not just set to no device*/
    if (slots[slot].card_type > NO_CARD_IN_SLOT) /*Card in slot*/
    {
        device_print("Send device reset on slot %d\r\n", slot);
        uart0_wait_until_tx_done(); // Wait until data has been transmitted.
        uart0_setup_read(1);
        uart0_write(portMAX_DELAY, &PRES, 1);
        slots[slot].device._ow_state = OW_WAIT_PRESENSE_SIGNAL;
        return 1;
    }
    else /*No card in slot*/
    {
        device_print("Cable detect no card in slot %d\r\n", slot);
        slots[slot].device._ow_state = OW_SEND_RESET_SIGNAL;
        slots[slot].device._dd_fail_count = DEVICE_DETECT_MISS_THRESHOLD;
        return 0;
    }
}

/**
 * @brief Here we check for the response from the reset we sent in the last
 * service state.
 * @param slot
 * @return If device is detected return 1, else return 0, if we need to wait for
 * read return 2
 */
static uint8_t reset_response(uint8_t slot)
{
    uint8_t data = OW_PRESENCE;

    /* Read */
    uart0_wait_until_tx_done();
    uart0_read(pdMS_TO_TICKS(1), &data);

    return data != OW_PRESENCE; // If a response occurred, the received data should NOT look like the presence signal.
}

static void write_srcatch_pad(uint8_t slot)
{
    ow_byte_wr(OW_SKIP_ROM);
    ow_byte_wr(OW_WR_SCRATCHPAD);

    ow_byte_wr((uint8_t)slots[slot].device._address);    // address low
    ow_byte_wr((uint8_t)(slots[slot].device._address >> 8));    // address high

    /* now write to the scratch pad*/
    for (uint8_t i = 0; i < SCRATCHPAD_SIZE; i++)
    {
        ow_byte_wr(slots[slot].device._p_ow_buffer[i]);
    }
    slots[slot].device._ow_state = OW_SEND_RESET_SIGNAL;
    slots[slot].device._ow_next_state = OW_READ_SRCATCH_PAD;
    device_print("Writing scratch pad on slot %d\r\n", slot);
}

static bool read_scratch_pad(uint8_t slot)
{
    uint8_t rd_val[SCRATCHPAD_SIZE];
    uint8_t PFFlag;
    bool matches = true;

    ow_byte_wr(OW_SKIP_ROM);
    ow_byte_wr(OW_RD_SCRATCHPAD);

    bool timed_out = false;

    slots[slot].device._TA1 = ow_byte_rd(&timed_out);
    if (timed_out) { return 0; }
    slots[slot].device._TA2 = ow_byte_rd(&timed_out);
    if (timed_out) { return 0; }

    slots[slot].device._ES = ow_byte_rd(&timed_out);
    if (timed_out) { return 0; }
    PFFlag = (slots[slot].device._ES & 0x20) >> 5;

    /* See if Write to Data to Scratch pad was written correctly*/
    if (PFFlag)
    {
        device_print("ERROR: PF Flag Set! Error EEPROM Write slot %d", slot);
        return 0;
    }

    for (uint8_t i = 0; i < SCRATCHPAD_SIZE; i++)
    {
        rd_val[i] = ow_byte_rd(&timed_out);
        if (timed_out) { return 0; }
        matches = matches && (rd_val[i] == slots[slot].device._p_ow_buffer[i]);
    }

    /*Compare it to the value written*/
    if (!matches)
    {
        device_print("ERROR: Scratch pad write error slot %d", slot);
        slots[slot].device._ow_state = OW_SEND_RESET_SIGNAL;
        slots[slot].device._ow_next_state = OW_SEND_RESET_SIGNAL;
        return 0;
    }
    slots[slot].device._ow_state = OW_SEND_RESET_SIGNAL;
    slots[slot].device._ow_next_state = OW_COPY_SRCATCH_PAD;
    device_print("Reading scratch pad on slot %d\r\n", slot);
    return 1;
}

static void copy_srcatch_pad(uint8_t slot)
{
    ow_byte_wr(OW_SKIP_ROM);
    ow_byte_wr(OW_COPY_SCRATCHPAD);

    ow_byte_wr(slots[slot].device._TA1);
    ow_byte_wr(slots[slot].device._TA2);
    ow_byte_wr(slots[slot].device._ES);

    // We need to set UART0 high to give power to the device.
    // Exact timing is unknown, so this is experimentally determined.
    vTaskDelay(DEVICE_DETECT_COPY_SCRATCHPAD_DELAY_TIME);
}

static void read_memory_device(uint8_t slot)
{
    //uint8_t family_code;
    //uint8_t crc;

    /*Read the serial number of the IC*/
    uint8_t * p_buffer = slots[slot].device._p_ow_buffer;

    ow_byte_wr(OW_RD_ROM);
    //ow_byte_wr(0);    // address low
    //ow_byte_wr(0);    // address high

    bool timed_out = false;
    for (uint8_t i = 0; i < sizeof(uint64_t); i++)
    {
        p_buffer[i] = ow_byte_rd(&timed_out);
        if (timed_out) { break; }
    }

    // save 1 byte family_code
    //family_code = rd_val[0];

    // save 6 bytes 48 bit serial_num

    // save 1 byte family_code
    //crc = rd_val[7];

    // TODO may want to check the return value using CRC

    device_print("slot %d serial num: %d\r\n", slot,
            slots[slot].device.info.serial_num);
}

static bool read_device(uint8_t slot)
{
    // uint8_t rd_val[sizeof(device_info)];

    // ow_byte_wr(OW_SKIP_ROM);
    // ow_byte_wr(OW_PRESENCE);
    // ow_byte_wr(0);
    // ow_byte_wr(0);

    // for (int i = 0; i < sizeof(device_info); i++)
    // {
    //     rd_val[i] = ow_byte_rd();
    // }

    // slots[slot].device.info.device_id = rd_val[0] | rd_val[1] << 8;

    // device_print("slot %d type: %d\r\n", slot,
    //         slots[slot].device.info.device_id);
    uint8_t * p_buffer = slots[slot].device._p_ow_buffer;
    uint16_t bytes_to_read = slots[slot].device._bytes_to_read;


    ow_byte_wr(OW_SKIP_ROM);
    ow_byte_wr(OW_RD_MEMORY);
    ow_byte_wr((uint8_t)slots[slot].device._address);
    ow_byte_wr((uint8_t)(slots[slot].device._address << 8));

    bool timed_out = false;
    for(uint8_t i = 0; i < bytes_to_read; ++i) {
        *(p_buffer++) = ow_byte_rd(&timed_out);
        if (timed_out) { break; }
    }

    return timed_out;
}

static bool service_ow(uint8_t slot)
{
    bool rt;
    switch (slots[slot].device._ow_state)
    {
    case OW_SEND_RESET_SIGNAL:
        /*Set uart to slow speed*/
        uart0_set_channel(slot, OW_RESET_PRESENCE_BAUD, slot + 7);
        rt = reset_send(slot);
        break;

    case OW_WAIT_PRESENSE_SIGNAL:
        /*Set uart to slow speed*/
        uart0_set_channel(slot, OW_RESET_PRESENCE_BAUD, slot + 7);
        if (reset_response(slot)) {
            // Device was detected
            slots[slot].device._dd_fail_count = 0;
            slots[slot].device._ow_state = slots[slot].device._ow_next_state;
            rt = slots[slot].device._ow_next_state != OW_SEND_RESET_SIGNAL; // If the next state is send reset, then we are done.
        } else {
            // Device was NOT detected.
            if (++slots[slot].device._dd_fail_count >= DEVICE_DETECT_MISS_THRESHOLD) {
                slots[slot].device._ow_state = OW_SEND_RESET_SIGNAL;
                slots[slot].device._ow_next_state = OW_SEND_RESET_SIGNAL;
                rt = 0;
            } else {
                // Need to try read again
                slots[slot].device._ow_state = OW_SEND_RESET_SIGNAL;
                rt = 1;
            }
        }

        break;

    case OW_WRITE_SRCATCH_PAD:
        /*Set uart to fast speed*/
        uart0_set_channel(slot, OW_READ_WRITE_BAUD, slot + 7);
        write_srcatch_pad(slot);
        rt = 1;
        break;

    case OW_READ_SRCATCH_PAD:
        /*Set uart to fast speed*/
        uart0_set_channel(slot, OW_READ_WRITE_BAUD, slot + 7);
        rt = read_scratch_pad(slot);
        break;

    case OW_COPY_SRCATCH_PAD:
        /*Set uart to fast speed*/
        uart0_set_channel(slot, OW_READ_WRITE_BAUD, slot + 7);
        copy_srcatch_pad(slot);
        rt = 0;
        break;

    case OW_READ_FLASH:
        /*Set uart to fast speed*/
        uart0_set_channel(slot, OW_READ_WRITE_BAUD, slot + 7);
        if (read_device(slot))
        {
            slots[slot].device._dd_fail_count = DEVICE_DETECT_MISS_THRESHOLD;
        }
        rt = 0;
        break;

    case OW_READ_MEMORY:
        /*Set uart to fast speed*/
        uart0_set_channel(slot, OW_READ_WRITE_BAUD, slot + 7);
        read_memory_device(slot);

        rt = 0;
        break;
    default:
        rt = 0;
    break;
    }

    return rt;
}

static void get_volatile_slot_config(Slots * const p_slot, bool * const p_is_static)
{
    xSemaphoreTake(p_slot->xSlot_Mutex, portMAX_DELAY);
    *p_is_static = p_slot->save.allow_device_detection == 0; // disable detection
    xSemaphoreGive(p_slot->xSlot_Mutex);
}


static pnp_status_t handle_fsm_v1 (const slot_nums slot, Device * const p_device, bool device_detected,
        const _device_eeprom_header_t * const p_header)
{
    pnp_status_t rt = pnp_status::NO_ERRORS;

    uint8_t rolling_checksum = p_header->version;

    uint16_t rolling_address = DEVICE_EEPROM_ADDRESS_V1_DATA;
    
    struct __attribute__((packed)){
        device_signature_t signature;
        char product_name[DEVICE_DETECT_PART_NUMBER_LENGTH];
        uint16_t device_entries_length;
        uint16_t config_entries_length;
    } start_info;

    uint8_t static_buffer[DEVICE_DETECT_STATIC_BUFFER_SIZE];            ///> Used when reading temporary data
    uint16_t bytes_to_read  = 0;
    bool saving = false;

    xSemaphoreTake(slots[slot].xSlot_Mutex, portMAX_DELAY);
    const slot_types SLOT_TYPE = slots[slot].card_type;                   ///> \note In the future, this may need to be converted somehow.
    xSemaphoreGive(slots[slot].xSlot_Mutex);

#define CHECK_TO_EXIT()        if (! device_detected) {using_ow = false; p_device->_dd_state = _DDS_NO_DEVICE; }
#define ROLL_STATIC_BUFFER()    for (int i = 0; i < p_device->_bytes_to_read; ++i) { rolling_checksum += static_buffer[i]; }

    do {
        bool using_ow = false;

        switch (p_device->_dd_state)
        {
        // Already handled in version-indepedent code.
        //case _DDS_NO_DEVICE:
        //case _DDS_UNKNOWN_DEVICE:
        //case _DDS_GET_VERSION:
        //case _DDS_BAD_DEVICE:
        case _DDS_CHECK_VERSION:
            // Because we entered this function, the version is valid.  Need to know what version-specific
            // state to transition to, though.

            p_device->_dd_state = _DDS_GET_HEADER;
        break;
        case _DDS_GET_HEADER:
            // Read in the header.
            p_device->_ow_next_state = OW_READ_FLASH;
            p_device->_address = DEVICE_EEPROM_ADDRESS_V1_HEADER;
            p_device->_p_ow_buffer = (uint8_t*)&start_info;
            p_device->_bytes_to_read = sizeof(start_info);
            using_ow = true;

            p_device->_dd_state = _DDS_CHECK_HEADER;

            CHECK_TO_EXIT();
        break;
        case _DDS_CHECK_HEADER:
            // Add to rolling checksum.
            for (size_t i = 0; i < sizeof(start_info); ++i)
            {
                rolling_checksum += ((uint8_t*)&start_info)[i];
            }

            xSemaphoreTake(slots[slot].xSlot_Mutex, portMAX_DELAY);
            memcpy(&p_device->info.signature, &start_info.signature, sizeof(start_info.signature));
            memcpy(&p_device->info.product_name, &start_info.product_name, DEVICE_DETECT_PART_NUMBER_LENGTH);
            xSemaphoreGive(slots[slot].xSlot_Mutex);


            // Check unbound memory.
            if ((uint32_t)(start_info.device_entries_length) + start_info.config_entries_length
                    + DEVICE_EEPROM_ADDRESS_V1_DATA < OW_EEPROM_SIZE
            ){

                // If config entires exist, then they needed to be loaded into memory.
                if (start_info.config_entries_length > 0)
                {
                    p_device->p_config_entries = new uint8_t[start_info.config_entries_length];
                    if (p_device->p_config_entries == NULL)
                    {
                        rt |= pnp_status::GENERAL_OW_ERROR;
                        p_device->_dd_state = _DDS_BAD_DEVICE;
                    }
                    p_device->p_config_entries_size = start_info.config_entries_length;
                }

                if (start_info.device_entries_length > 0)
                {
                    p_device->_dd_state = _DDS_GET_DEVICE_ENTRY_HEADER;
                }
                else if (start_info.config_entries_length > 0)
                {
                    p_device->_dd_state = _DDS_GET_CONFIG_ENTRIES;
                }
                else
                {
                    p_device->_dd_state = _DDS_VALIDATE_CHECKSUM;
                }
            } else {
                // If the memory goes out of bounds, the device needs to be
                // reprogrammed.
                rt |= pnp_status::GENERAL_OW_ERROR;
                p_device->_dd_state = _DDS_BAD_DEVICE;
            }
        break;
        case _DDS_GET_DEVICE_ENTRY_HEADER:
            // Read in device entry header.
            p_device->_ow_next_state = OW_READ_FLASH;
            p_device->_address = rolling_address;
            p_device->_p_ow_buffer = static_buffer;
            p_device->_bytes_to_read = 4;

            rolling_address += 4;

            using_ow = true;

            p_device->_dd_state = _DDS_CHECK_DEVICE_ENTRY_HEADER;

            CHECK_TO_EXIT();
        break;
        case _DDS_CHECK_DEVICE_ENTRY_HEADER:
            // Add device entry header into rolling checksum.
            ROLL_STATIC_BUFFER();

            {
                slot_types st;
                memcpy(&st, static_buffer, 2);
                memcpy(&bytes_to_read, static_buffer + 2, 2);

                // Read in the device entry.
                p_device->_ow_next_state = OW_READ_FLASH;
                p_device->_address = rolling_address;

                p_device->_dd_state = _DDS_PROCESS_DEVICE_ENTRY;

                using_ow = true;

                if (st == SLOT_TYPE) {
                    // Matching config, so save it this time.
                    saving = true;

                    p_device->p_slot_config = static_cast<config_signature_t*>(pvPortMalloc(bytes_to_read));
                    if (p_device->p_slot_config == NULL)
                    {
                        using_ow = false;
                        rt |= pnp_status::GENERAL_OW_ERROR;
                        p_device->_dd_state = _DDS_BAD_DEVICE;
                    }
                    p_device->p_slot_config_size = bytes_to_read;
                    p_device->_p_ow_buffer = reinterpret_cast<uint8_t*>(p_device->p_slot_config);
                    p_device->_bytes_to_read = bytes_to_read;
                } else {
                    p_device->_p_ow_buffer = static_buffer;
                    p_device->_bytes_to_read = (bytes_to_read > DEVICE_DETECT_STATIC_BUFFER_SIZE) ? DEVICE_DETECT_STATIC_BUFFER_SIZE : bytes_to_read;
                }

                rolling_address += p_device->_bytes_to_read;
            }

            CHECK_TO_EXIT();
        break;
        case _DDS_PROCESS_DEVICE_ENTRY:
            // Add to rolling checkum.
            if (saving) {
                saving = false;
                uint8_t * const p_buff = (uint8_t*)p_device->p_slot_config;
                for (int i = 0; i < p_device->p_slot_config_size; ++i)
                {
                    rolling_checksum += p_buff[i];
                }
            } else {
                ROLL_STATIC_BUFFER();
            }

            bytes_to_read -= p_device->_bytes_to_read;
            
            if (bytes_to_read > 0) {
                // Continue to read in.
                p_device->_ow_next_state = OW_READ_FLASH;
                p_device->_address = rolling_address;
                p_device->_p_ow_buffer = static_buffer;
                p_device->_bytes_to_read = (bytes_to_read > DEVICE_DETECT_STATIC_BUFFER_SIZE) ? DEVICE_DETECT_STATIC_BUFFER_SIZE : bytes_to_read;

                rolling_address += p_device->_bytes_to_read;

                using_ow = true;
            } else {
                if (rolling_address >= DEVICE_EEPROM_ADDRESS_V1_DATA + start_info.device_entries_length) {
                    if (start_info.config_entries_length > 0) {
                        p_device->_dd_state = _DDS_GET_CONFIG_ENTRIES;
                    } else {
                        p_device->_dd_state = _DDS_VALIDATE_CHECKSUM;
                    }
                } else {
                    p_device->_dd_state = _DDS_GET_DEVICE_ENTRY_HEADER;
                }
            }
        break;
        case _DDS_GET_CONFIG_ENTRIES:
            // Read in the everything
            p_device->_ow_next_state = OW_READ_FLASH;
            p_device->_address = rolling_address;
            p_device->_p_ow_buffer = p_device->p_config_entries;
            p_device->_bytes_to_read = start_info.config_entries_length;

            using_ow = true;

            p_device->_dd_state = _DDS_CHECK_CONFIG_ENTIRES;
            CHECK_TO_EXIT();
        break;
        case _DDS_CHECK_CONFIG_ENTIRES:
            for (int i = 0; i < start_info.config_entries_length; ++i)
            {
                rolling_checksum += p_device->p_config_entries[i];
            }

            p_device->_dd_state = _DDS_VALIDATE_CHECKSUM;
        break;

        case _DDS_VALIDATE_CHECKSUM:

            if (rolling_checksum == p_header->checksum) {
                xSemaphoreTake(slots[slot].xSlot_Mutex, portMAX_DELAY);

                // Check that the serial number matches.
                if (OW_SERIAL_NUMBER_WILDCARD == slots[slot].save.allowed_device_serial_number ||
                    p_device->info.serial_num == slots[slot].save.allowed_device_serial_number) {
                    // Check if the slot could even possibly drive the device.
                    const bool COMPATIBLE_CARD = slot_is_compatible(slots[slot].card_type, start_info.signature.slot_type);
                    if (COMPATIBLE_CARD)
                    {
                        // Check that the device is allowed.
                        for (uint16_t allowed_index = 0; allowed_index < slots[slot].save.num_allowed_devices; ++allowed_index)
                        {
                            if (
                                    slots[slot].p_allowed_devices[allowed_index].slot_type == start_info.signature.slot_type &&
                                    (
                                        slots[slot].p_allowed_devices[allowed_index].device_id == start_info.signature.device_id ||
                                        slots[slot].p_allowed_devices[allowed_index].device_id == 0xFFFF    // Wildcard allow
                                    )
                                )
                            {
#if DDEnableGates
                                xGateOpen(p_device->connection_gate);
#endif
                                p_device->_dd_state = _DDS_PASSED;
                                break;
                            }
                        }
                    }
                    if (p_device->_dd_state != _DDS_PASSED)
                    {
                        rt |= pnp_status::SIGNATURE_NOT_ALLOWED;
                        if (! COMPATIBLE_CARD) {
                            rt |= pnp_status::INCOMPATIBLE_CARD_TYPE;
                        }
                    }
                } else {
                    rt |= pnp_status::SERIAL_NUM_MISMATCH;
                }

                xSemaphoreGive(slots[slot].xSlot_Mutex);
            } else {
                rt |= pnp_status::OW_CORRUPTION;
            }
                

            // If nothing worked (no state change), set to bad state.
            if (p_device->_dd_state == _DDS_VALIDATE_CHECKSUM) {
                rt |= pnp_status::GENERAL_OW_ERROR;
                p_device->_dd_state = _DDS_BAD_DEVICE;
            }

        break;
        default:
            // Unknown state.  Stop the FSM.
            using_ow = false;
            rt |= pnp_status::GENERAL_OW_ERROR;
            p_device->_dd_state = _DDS_BAD_DEVICE;
        break;
        }

        // Process one-wire until done.
        if (using_ow)
        {
            p_device->_ow_state = OW_SEND_RESET_SIGNAL;
            xSemaphoreTake(xUART0_Semaphore, portMAX_DELAY);
            while (service_ow(slot)) {} ///> Keep servicing one-wire until it is done using it (returns 0).
            xSemaphoreGive(xUART0_Semaphore);
            device_detected = p_device->_dd_fail_count == 0;
        }
    } while (p_device->_dd_state != _DDS_NO_DEVICE &&
             p_device->_dd_state != _DDS_BAD_DEVICE &&
             p_device->_dd_state != _DDS_STATIC_DEVICE &&
             p_device->_dd_state != _DDS_PASSED
            );
    
    return rt;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

static void task_device_detect(void * p_params)
{
    // Initialize tick counter.
    TickType_t now = xTaskGetTickCount();

    // Static buffer to indicating if device-detect is disabled.
    bool dd_disabled[NUMBER_OF_BOARD_SLOTS];

    // Read in volatile slot config.
    for (slot_nums slot = SLOT_1; slot < NUMBER_OF_BOARD_SLOTS; ++slot) {
        get_volatile_slot_config(slots + slot, dd_disabled + slot);
    }

    // Create and assign heartbeat watchdog to supervisor.
    heartbeat_watchdog watchdog(DEVICE_DETECT_HEARTBEAT_INTERVAL);
    supervisor_add_watchdog(&watchdog);

    for (;;)
    {

        // Handle the command queues for each device.
        for (slot_nums slot = SLOT_1; slot < NUMBER_OF_BOARD_SLOTS; ++slot) {
            Device * const p_device = &slots[slot].device;
            _device_detect_command_t cmd;
            while (xQueueReceive(p_device->_dd_command_queue, &cmd, 0))
            {
                OW_Programming_Error_t err = OWPE_OKAY;
                switch (cmd.operation)
                {
                case _DDC_RESET_FSM:
                    // Force-fail device detection
                    p_device->_dd_fail_count = 1;
                    p_device->_dd_abort_device_detection = true;

                    // Reload volatile slot config.
                    get_volatile_slot_config(slots + slot, dd_disabled + slot);
                break;

                case _DDC_ENABLE_PROGRAMING:
                    if (p_device->_dd_state == _DDS_PROGRAMMING) {
                        // Already programming. Return an error.
                        err = OWPE_ALREADY_PROGRAMMING;
                    } else if (p_device->_dd_state == _DDS_NO_DEVICE) {
                        // No device is in the slot.  Return an error.
                        err = OWPE_NO_DEVICE;
                    } else if (dd_disabled[slot]) {
                        // Static device configuration.  Do NOT do device detection.
                        err = OWPE_DEVICE_DOES_NOT_SUPPORT_OW;
                    } else {
                        // Force-fail device detection
                        p_device->_dd_fail_count = 1;
                        p_device->_dd_abort_device_detection = true;

                        // Enable programming.
                        p_device->_dd_programming = _RAW_PROGRAMMING;
                    }
                break;

                case _DDC_STOP_PROGRAMMING:
                    if (p_device->_dd_state != _DDS_PROGRAMMING) {
                        // Not programming.  Return an error.
                        err = OWPE_NOT_PROGRAMMING;
                    } else if (p_device->_dd_state == _DDS_NO_DEVICE) {
                        // No device is in the slot.  Return an error.
                        err = OWPE_NO_DEVICE;
                    } else if (dd_disabled[slot]) {
                        // Static device configuration.  Do NOT do device detection.
                        err = OWPE_DEVICE_DOES_NOT_SUPPORT_OW;
                    } else {
                        // Disable programming.
                        p_device->_dd_programming = _NOT_PROGRAMMING;
                    }
                break;

                case _DDC_CHECK_PROGRAMMING:
                    if (p_device->_dd_state != _DDS_PROGRAMMING) {
                        err = OWPE_NOT_PROGRAMMING;
                    } else if (p_device->_dd_state == _DDS_NO_DEVICE) {
                        err = OWPE_NO_DEVICE;
                    } else if (dd_disabled[slot]) {
                        err = OWPE_DEVICE_DOES_NOT_SUPPORT_OW;
                    } else {
                        err = OWPE_ALREADY_PROGRAMMING;
                    }
                break;

                case _DDC_PROGRAM:
                    if (p_device->_dd_state != _DDS_PROGRAMMING) {
                        // Not programming.  Return an error.
                        err = OWPE_NOT_PROGRAMMING;
                    } else if (p_device->_dd_state == _DDS_NO_DEVICE) {
                        // No device is in the slot.  Return an error.
                        err = OWPE_NO_DEVICE;
                    } else if (dd_disabled[slot]) {
                        // Static device configuration.  Do NOT do device detection.
                        err = OWPE_DEVICE_DOES_NOT_SUPPORT_OW;
                    } else if (cmd.parameter2 > DEVICE_DETECT_PROGRAMMING_BUFFER_SIZE) {
                        // Passed buffer is too big.
                        err = OWPE_PACKET_TOO_LARGE;
                    } else if (cmd.parameter2 + p_device->_dd_programming_address > OW_EEPROM_SIZE) {
                        // Overflow of EEPROM would occur.
                        err = OWPE_OUT_OF_MEMORY;
                    } else {
                        // Copy data into programming buffer.
                        memcpy(p_device->_dd_p_programming_buffer, cmd.parameter1.ptr, cmd.parameter2);
                        p_device->_dd_data_in_buffer = cmd.parameter2;

                        // Pad additional bytes with empty bytes (0xFF).
                        const uint8_t ADDITIONAL_EMPTY_BYTES =
                            (cmd.parameter2 % SCRATCHPAD_SIZE == 0)
                            ? 0
                            : (8 - cmd.parameter2 % SCRATCHPAD_SIZE);
                        for (uint16_t i = 0; i < ADDITIONAL_EMPTY_BYTES; ++i)
                        {
                            p_device->_dd_p_programming_buffer[cmd.parameter2 + i] = DUMMY_HIGH;
                        }
                    }
                break;
                default:
                break;
                }

                if (cmd.p_callback != NULL)
                {
                    switch (cmd.operation)
                    {
                    case _DDC_PROGRAM:
                        // Callback is in form "void FUNC (void * const, const OW_Programming_Error_t, void * const)"
                        ((void (*) (void * const, const OW_Programming_Error_t, void * const))cmd.p_callback)(
                            cmd.p_callback_param,
                            err,
                            cmd.parameter1.ptr
                        );
                    break;
                    default:
                        // Callback is in form "void FUNC (void * const, const OW_Programming_Error_t)"
                        ((void (*) (void * const, const OW_Programming_Error_t))cmd.p_callback)(
                            cmd.p_callback_param,
                            err
                        );
                    break;
                    }
                }
            }
        }

        
        // Check for device connections, unless told not to.
        xSemaphoreTake(xUART0_Semaphore, portMAX_DELAY);
        for(slot_nums slot = SLOT_1; slot < NUMBER_OF_BOARD_SLOTS; ++slot)
        {
            if (slots[slot].device._dd_abort_device_detection) {
                slots[slot].device._dd_abort_device_detection = false;
            } else {
                if (dd_disabled[slot]) {
                    // Pass detect device
                    slots[slot].device._dd_fail_count = 0;
                } else {
                    // Set-up one-wire to send a reset signal and exit on detection (next state = reset signal).
                    slots[slot].device._ow_state = OW_SEND_RESET_SIGNAL;
                    slots[slot].device._ow_next_state = OW_SEND_RESET_SIGNAL;
                    slots[slot].device._dd_fail_count = 0; // Need this to reset to start, or it won't send properly.

                    // Service device detection.
                    while ((slots[slot].device._ow_state == OW_SEND_RESET_SIGNAL || 
                            slots[slot].device._ow_state == OW_WAIT_PRESENSE_SIGNAL) && service_ow(slot))
                    {}
                    // Because this loops, if _dd_fail_count is zero, the device was detected
                    // If a device was detected, it can either be set to reset again or go into some other command.
                }
            }
            uart0_wait_until_tx_done();
        }
        xSemaphoreGive(xUART0_Semaphore);


        // Handle device-detection tasks for all slot cards.
        for(slot_nums slot = SLOT_1; slot < NUMBER_OF_BOARD_SLOTS; ++slot)
        {
            pnp_status_t status_flags = 0;

            Device * const p_device = &slots[slot].device;
            const bool DEVICE_DETECTED = p_device->_dd_fail_count == 0;
            // uint8_t current_checksum = 0;

            // Saved variables
            _device_eeprom_header_t header;
            uint64_t read_serial_number;
            uint8_t scratchpad_used = 0;
            uint16_t programming_buffer_index = 0;

            // Handle generic states
            do
            {
                bool handle_ow = false;

                // Process device-detect state machine
                switch (p_device->_dd_state) {
                case _DDS_NO_DEVICE:
                    if (DEVICE_DETECTED) {
                        p_device->_dd_delay_counter = DEVICE_DETECT_DELAY_LOOPS;
                        p_device->_dd_state = _DDS_CONNECTION_DELAY;
                    }
                    if (p_device->info.serial_num != NO_DEVICE_SERIAL_NUMBER)
                    {
                        set_ow_serial_number_raw(slot, NO_DEVICE_SERIAL_NUMBER);
                    }
                break;

                case _DDS_CONNECTION_DELAY:
                    // A delay to allow for the plug to be fully inserted.
                    if (--p_device->_dd_delay_counter == 0)
                    {
                        p_device->_dd_state = _DDS_UNKNOWN_DEVICE;
                    }

                    break;

                case _DDS_UNKNOWN_DEVICE:
                    if (dd_disabled[slot]) {
                        // Device detection is disabled, so drive what's in memory.

                        p_device->is_save_structure = false;
                        p_device->p_slot_config = NULL;
                        p_device->p_slot_config_size = 0;

#if DDEnableGates
                        xGateOpen(p_device->connection_gate);
#endif

                        p_device->_dd_state = _DDS_STATIC_DEVICE;
                    } else if (p_device->_dd_programming != _NOT_PROGRAMMING) {
                        // Allocate buffer and initialize system.
                        switch (p_device->_dd_programming)
                        {
                        case _RAW_PROGRAMMING:
                            p_device->_dd_p_programming_buffer = new uint8_t[DEVICE_DETECT_PROGRAMMING_BUFFER_SIZE];
                            p_device->_dd_programming_address = 0;
                            p_device->_dd_data_in_buffer = 0;
                        break;
                        default:
                        break;
                        }

                        p_device->_dd_state = _DDS_PROGRAMMING;
                    } else {
                        // Read in one-wire serial number.
                        p_device->_ow_state = OW_SEND_RESET_SIGNAL;
                        p_device->_ow_next_state = OW_READ_MEMORY;
                        // p_device->_address           N/A
                        p_device->_p_ow_buffer = reinterpret_cast<uint8_t*>(&read_serial_number);
                        // p_device->_bytes_to_read     Will always read in 8 bytes.
                        p_device->_dd_state = _DDS_READ_HEADER;
                        handle_ow = true;
                    }
                break;

                case _DDS_READ_HEADER:
                    // Save read serial number.
                    set_ow_serial_number(slot, read_serial_number);

                    // Read in one-wire version and checksum
                    p_device->_ow_state = OW_SEND_RESET_SIGNAL;
                    p_device->_ow_next_state = OW_READ_FLASH;
                    p_device->_address = DEVICE_EEPROM_ADDRESS_VERSION;
                    p_device->_p_ow_buffer = (uint8_t*)&header;
                    p_device->_bytes_to_read = sizeof(header);
                    p_device->_dd_state = _DDS_CHECK_VERSION;
                    handle_ow = true;
                break;

                case _DDS_PROGRAMMING:
                    if (! DEVICE_DETECTED) {
                        p_device->_dd_programming = _NOT_PROGRAMMING;
                        p_device->_dd_state = _DDS_NO_DEVICE;
                        vPortFree(p_device->_dd_p_programming_buffer);
                    } else if (p_device->_dd_programming == _NOT_PROGRAMMING) {
                        // No longer programming.  Continue with device-detect.
                        p_device->_dd_state = _DDS_UNKNOWN_DEVICE;
                        vPortFree(p_device->_dd_p_programming_buffer);
                    } else if (p_device->_dd_data_in_buffer > 0) {
                        // Data is available to be written.
                        // Calculate how much of the scratchpad is being used.
                        scratchpad_used = (p_device->_dd_data_in_buffer > 8) ? 8 : p_device->_dd_data_in_buffer;
                        
                        p_device->_ow_state = OW_SEND_RESET_SIGNAL;
                        p_device->_ow_next_state = OW_WRITE_SRCATCH_PAD;
                        p_device->_address = p_device->_dd_programming_address;
                        p_device->_p_ow_buffer = p_device->_dd_p_programming_buffer + programming_buffer_index;
                        p_device->_dd_state = _DDS_PROGRAMMING_VERIFY;
                        handle_ow = true;
                    }
                break;

                case _DDS_PROGRAMMING_VERIFY:
                    // Check to see if the state made it to copy (write to flash).
                    if (p_device->_ow_state == OW_COPY_SRCATCH_PAD) {
                        programming_buffer_index += 8;

                        p_device->_dd_data_in_buffer -= scratchpad_used;
                        p_device->_dd_programming_address += scratchpad_used;
                    }

                    p_device->_dd_state = _DDS_PROGRAMMING;
                break;

                case _DDS_STATIC_DEVICE:
                    // The only time a static device would not be detected is if device detection was reset.
                    if (! DEVICE_DETECTED) {
#if DDEnableGates
                        xGateClose(p_device->connection_gate);
#endif
                        p_device->_dd_state = _DDS_NO_DEVICE;
                    }
                break;

                case _DDS_BAD_DEVICE:
                    // Can't set bad status here, only when transitioning in.
                    if (! DEVICE_DETECTED) {
                        p_device->_dd_state = _DDS_NO_DEVICE;
                    }
                break;
                case _DDS_PASSED:
                    if (! DEVICE_DETECTED) {
#if DDEnableGates
                        xGateClose(p_device->connection_gate);
#endif
                        p_device->_dd_state = _DDS_NO_DEVICE;
                    }
                break;
                default:
                break;
                }

                // Process one-wire until done.
                if (handle_ow) {
                    xSemaphoreTake(xUART0_Semaphore, portMAX_DELAY);
                    // Enable data available interrupt
                    while (service_ow(slot)) {} ///> Keep servicing one-wire until it is done using it (returns 0).
                    // Disable data available interrupt
                    xSemaphoreGive(xUART0_Semaphore);
                }
#if 0
            } while (p_device->_dd_state != _DDS_NO_DEVICE &&
                     p_device->_dd_state != _DDS_BAD_DEVICE &&
                     p_device->_dd_state != _DDS_STATIC_DEVICE &&
                     p_device->_dd_state != _DDS_PROGRAMMING &&
                     p_device->_dd_state != _DDS_CHECK_VERSION
                    );
#endif
            } while (p_device->_dd_state == _DDS_UNKNOWN_DEVICE ||
                    p_device->_dd_state == _DDS_READ_HEADER ||
                     (p_device->_dd_state == _DDS_PROGRAMMING && p_device->_dd_data_in_buffer > 0) ||
                     p_device->_dd_state == _DDS_PROGRAMMING_VERIFY
                    );

            // Do not go into device-specific FSM unless a state
            // can transition into it.
            if (p_device->_dd_state != _DDS_NO_DEVICE &&
                p_device->_dd_state != _DDS_CONNECTION_DELAY &&
                p_device->_dd_state != _DDS_BAD_DEVICE &&
                p_device->_dd_state != _DDS_STATIC_DEVICE &&
                p_device->_dd_state != _DDS_PASSED &&
                p_device->_dd_state != _DDS_PROGRAMMING)
            {
                pnp_status_t sb = pnp_status::NO_ERRORS;
                switch(header.version) {
                case (DEVICE_DETECT_V1):
                    sb = handle_fsm_v1(slot, p_device, DEVICE_DETECTED, &header);
                break;
                default: // Invalid version
                    sb = pnp_status::GENERAL_OW_ERROR | pnp_status::UNKNOWN_OW_VERSION;
                    p_device->_dd_state = _DDS_BAD_DEVICE;
                break;
                }

                status_flags |= sb;
            }

            // Update the status bits.
            if (status_flags & pnp_status::GENERAL_OW_ERROR) {
                // If a general ow error status is reported, save the status flags.
                p_device->_dd_previous_status_flags = static_cast<uint8_t>(status_flags);
            }
            switch(p_device->_dd_state)
            {
                case _DDS_NO_DEVICE:
                case _DDS_CONNECTION_DELAY:
                    status_flags = pnp_status::NO_DEVICE_CONNECTED;
                break;
                case _DDS_STATIC_DEVICE:
                    status_flags = pnp_status::NO_ERRORS;
                break;
                case _DDS_BAD_DEVICE:
                    status_flags = p_device->_dd_previous_status_flags;
                break;
                default:
                break;
            }

            xSemaphoreTake(slots[slot].xSlot_Mutex, portMAX_DELAY);
            slots[slot].pnp_status = (slots[slot].pnp_status & PNP_STATUS_MASK) | status_flags;
            xSemaphoreGive(slots[slot].xSlot_Mutex);

            uart0_wait_until_tx_done();
        }

        // Schedule next time to run
        watchdog.beat();
        vTaskDelayUntil(&now, DEVICE_DETECT_UPDATE_INTERVAL);
    }
}


void device_detect_init(void)
{
    // Setup Task structures
    for (uint8_t slot = 0; slot < NUMBER_OF_BOARD_SLOTS ; slot++)
    {
        // Device Info Fields
        slots[slot].device.info.signature.slot_type = END_CARD_TYPES;
        slots[slot].device.info.signature.device_id = DEVICE_CHECK_INCOMPLETE;
        slots[slot].device.info.serial_num = 0;
#if DDEnableGates
        slots[slot].device.connection_gate = xGateCreate();
        if (slots[slot].device.connection_gate == INT_NULL) {
            /// \todo Throw an error.
        }
#endif
#ifdef USE_RTOS_DEBUGGING
        char * name = new char[8]; // This needs to be kept in memory for the program's duration.
        strcpy(name, "SLOT 1G");
        name[5] += slot;
        NameQueueObject(reinterpret_cast<QueueHandle_t>(slots[slot].device.connection_gate), name);
#endif

        // Public Device-Detect Fields
        // Field is_save_structure does not need to be configured.
        slots[slot].device.p_slot_config = NULL;
        slots[slot].device.p_slot_config_size = 0;
        slots[slot].device.p_config_entries = NULL;
        slots[slot].device.p_config_entries_size = 0;
    

        // On-Wire Fields
        slots[slot].device._ow_state = OW_SEND_RESET_SIGNAL;
        slots[slot].device._ow_next_state = OW_SEND_RESET_SIGNAL;
        // Fields _TA1, _TA2, and _ES do not need to be configured.
        slots[slot].device._address = 0;
        slots[slot].device._p_ow_buffer = NULL;
        slots[slot].device._bytes_to_read = 0;

        // Device-Detect FSM
        slots[slot].device._dd_state = _DDS_NO_DEVICE;
        slots[slot].device._dd_fail_count = 0;
        slots[slot].device._dd_abort_device_detection = false;
        slots[slot].device._dd_programming = _NOT_PROGRAMMING;
        slots[slot].device._dd_previous_status_flags = static_cast<uint8_t>(pnp_status::NO_ERRORS);

        slots[slot].device._dd_command_queue = xQueueCreate(2, sizeof(_device_detect_command_t));
        configASSERT(slots[slot].device._dd_command_queue);
#ifdef USE_RTOS_DEBUGGING
        name = new char[8]; // This needs to be kept in memory for the program's duration.
        strcpy(name, "SLOT 1Q");
        name[5] += slot;
        NameQueueObject(slots[slot].device._dd_command_queue, name);
#endif
    }

    if (xTaskCreate((TaskFunction_t)(&task_device_detect), "Device Detect",
            TASK_DEVICE_DETECT_STACK_SIZE, NULL, TASK_DEVICE_DETECT_STACK_PRIORITY, &xDeviceDetection) != pdPASS)
    {
        debug_print("Failed to create device-detect task\r\n");
    }

}

extern "C" void device_detect_reset(Device * const p_device)
{
    _device_detect_command_t cmd;
    cmd.operation = _DDC_RESET_FSM;
    cmd.p_callback = nullptr; // No callback needed.
    xQueueSend(p_device->_dd_command_queue, &cmd, portMAX_DELAY);
}

extern "C" void device_detect_change_programming(Device * const p_device,
    const bool enable_programming, void (*p_callback)(void * const, const OW_Programming_Error_t),
    void * const p_callback_param)
{
    _device_detect_command_t cmd;
    cmd.operation = (enable_programming) ? _DDC_ENABLE_PROGRAMING : _DDC_STOP_PROGRAMMING;
    cmd.p_callback = reinterpret_cast<void*>(p_callback);
    cmd.p_callback_param = reinterpret_cast<void*>(p_callback_param);
    xQueueSend(p_device->_dd_command_queue, &cmd, portMAX_DELAY);
}

extern "C" void device_detect_check_programming(Device * const p_device,
    void (*p_callback)(void * const, const OW_Programming_Error_t), void * const p_callback_param)
{
    _device_detect_command_t cmd;
    cmd.operation = _DDC_CHECK_PROGRAMMING;
    cmd.p_callback = reinterpret_cast<void*>(p_callback);
    cmd.p_callback_param = reinterpret_cast<void*>(p_callback_param);
    xQueueSend(p_device->_dd_command_queue, &cmd, portMAX_DELAY);
}

extern "C" void device_detect_handle_ow_programming_packet(
    Device * const p_device, uint8_t * const p_data_to_write,
    uint16_t length_of_data, void (*p_callback)(void * const, const OW_Programming_Error_t, void * const),
    void * const p_callback_param)
{
    _device_detect_command_t cmd;
    cmd.operation = _DDC_PROGRAM;
    cmd.parameter2 = length_of_data;
    cmd.parameter1.ptr = p_data_to_write;
    cmd.p_callback = reinterpret_cast<void*>(p_callback);
    cmd.p_callback_param = reinterpret_cast<void*>(p_callback_param);
    xQueueSend(p_device->_dd_command_queue, &cmd, portMAX_DELAY);
}

//EOF

