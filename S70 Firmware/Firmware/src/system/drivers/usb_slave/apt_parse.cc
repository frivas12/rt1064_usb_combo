/**
 * @file apt_parse.c
 *
 * @brief Functions for ???
 *
 */

#include <asf.h>
#include "UsbCore.h"
#include "apt-command.hh"
#include "apt-parsing.hh"
#include "hid-mapping.hh"
#include "hid_in.h"
#include "hid_out.h"
#include "hid_report.h"
#include "itc-service.hh"
#include "ptr-to-span.hh"
#include "spi-transfer-handle.hh"
#include "usb_slave.h"
#include <apt_parse.h>
#include <cstdint>
#include "usb_host.h"
#include "usb_device.h"
#include "hid.h"
#include "ftdi.h"
#include "cpld_program.h"
#include "apt.h"
#include "Debugging.h"
#include "sys_task.h"
#include "25lc1024.h"
#include "string.h"
#include "device_detect.h"
#include "version.h"
#include "target-config.h"

#include "../log/log.h"
#include "cpld.h"
#include "helper.h"
#include "log.h"
#include "supervisor.h"
#include "apt_traits.tcc"

#include "lock_guard.hh"
#include "lut_manager.hh"

#include "efs.hh"

#include "joystick_map_in.hh"
#include "joystick_map_out.hh"

/****************************************************************************
 * Private Data
 ****************************************************************************/
// (sbenish)  Something weird was going on at 125.
// A fully-filled command wouldn't send for some reason.
static constexpr uint8_t RESPONSE_BUFFER_SIZE = 100;

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static void parse(USB_Slave_Message *slave_message);

// Helper function that allows for the stand response (save the responding command) to both the new and old
// hw info commands.
static std::size_t hw_info_response(uint8_t p_buffer[USB_SLAVE_BUFFER_SIZE],
                                    const uint16_t get_command);

static Hid_Mapping_control_in translate(const drivers::apt::joystick_map_in::payload_type& value);
static drivers::apt::joystick_map_in::payload_type translate(const Hid_Mapping_control_in& value);
static Hid_Mapping_control_out translate(const drivers::apt::joystick_map_out::payload_type& value);
static drivers::apt::joystick_map_out::payload_type translate(const Hid_Mapping_control_out& value);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
union CallbackUnion
{
    void * ptr;
    slot_types slot_type;
};

static void callback_ow_response(void * const p_param, const OW_Programming_Error_t err) {
    // Construct an artificial message to send back to the slot, which will forward it to the host.
    CallbackUnion un; un.ptr = p_param;
    const slot_types SLOT = un.slot_type;
    USB_Slave_Message slave_message;

    slave_message.extended_data_buf[0] = SLOT;
    slave_message.extended_data_buf[1] = 0x00;
    slave_message.extended_data_buf[2] = err;
    slave_message.ucMessageID = MGMSG_OW_GET_PROGRAMMING;
    slave_message.xUSB_Slave_TxSemaphore = xUSB_Slave_TxSemaphore;
    slave_message.write = udi_cdc_write_buf;

    parse(&slave_message);
}

static void callback_ow_program(void * const p_param, const OW_Programming_Error_t err, void * const p_buffer) {
    // Free the buffer.
    vPortFree(p_buffer);

    callback_ow_response(p_param, err);
}

/**
 * @brief Parse APT commands that are specific to the board not a card in a slot
 *
 * Takes xUSB_Slave_TxSemaphore - sending data on the USB bus
 *
 * @param buf : The pointer buffer which holds the data to parse.
 * @param slave_message_id : This is the massage structure pass from the USB salve task
 */
static void parse(USB_Slave_Message *slave_message)
{
    uint8_t response_buffer[RESPONSE_BUFFER_SIZE];
    uint8_t length = 0;
    // uint32_t resp;
    bool need_to_reply = false;

    //bool send_log_flag = false;
    
    const std::span<std::byte, drivers::usb::apt_response::RESPONSE_BUFFER_SIZE>
        extended_data_span = ptr2span
            <drivers::usb::apt_response::RESPONSE_BUFFER_SIZE>
            (reinterpret_cast<std::byte*>(response_buffer + 6));
    drivers::usb::apt_response_builder response_builder (extended_data_span);
    const drivers::usb::apt_basic_command command_proxy (*slave_message);

    switch (slave_message->ucMessageID)
    {
        case MGMSG_SET_SER_TO_EEPROM:
            {
                // Added variable cipher to obfuscate the key from code analysis.
#define CIPER_0         3
#define CIPER_1         159
#define CIPER_2         240
#define CIPER_3         88
                char THOR_KEY[] = {
                        (char)('T' + CIPER_0),
                        (char)('H' + CIPER_1),
                        (char)('O' + CIPER_2),
                        (char)('R' + CIPER_3)
                };

                // Subtracting message from key.
                for (int i = 0; i < 4; ++i)
                {
                    THOR_KEY[i] -= slave_message->extended_data_buf[i];
                }

                const bool OPEN = THOR_KEY[0] == CIPER_0 &&
                        THOR_KEY[1] == CIPER_1 &&
                        THOR_KEY[2] == CIPER_2 &&
                        THOR_KEY[3] == CIPER_3;

#undef CIPER_0
#undef CIPER_1
#undef CIPER_2
#undef CIPER_3

                if(OPEN && slave_message->ExtendedData_len == 4 + USB_DEVICE_GET_SERIAL_NAME_LENGTH){

                        xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
                        set_board_serial_number((char*)slave_message->extended_data_buf + 4);
                        xSemaphoreGive(xSPI_Semaphore);

                        //Send ACK here
                        response_buffer[0] = (uint8_t)MGMSG_GET_SER_STATUS;
                        response_buffer[1] = (uint8_t)(MGMSG_GET_SER_STATUS >> 8);
                        response_buffer[2] = 1;
                        response_buffer[3] = 0;
                        response_buffer[4] = HOST_ID | 0x80;
                        response_buffer[5] = MOTHERBOARD_ID;
                        response_buffer[6] = 1;

                        need_to_reply = true;
                        length = 7;
                }
            }
        break;
    case MGMSG_HW_REQ_INFO: /* 0x0005 */
        need_to_reply = true;
        length = hw_info_response(response_buffer, MGMSG_HW_GET_INFO);
        //		debug_print("%H APT command.\r\n", slave_message->ucMessageID);
    break;

    case MGMSG_MOD_IDENTIFY:  /// 0x0223
        supervisor_identify(static_cast<slot_nums>(slave_message->param1));
    break;

    case MGMSG_GET_UPDATE_FIRMWARE:
        set_firmware_load_count();
        setup_and_send_log(slave_message, MOTHERBOARD_ID, LOG_REPEAT, SYSTEM_LOG_TYPE,
                SYSTEM_UPDATE_CPLD_LOG, 0, 0, 0);
    break;

    case MGMSG_MCM_HW_REQ_INFO: /* 0x4000 */
        need_to_reply = true;
        length = hw_info_response(response_buffer, MGMSG_MCM_HW_GET_INFO);
//		debug_print("%H APT command.\r\n", slave_message->ucMessageID);
    break;

    case MGMSG_CPLD_UPDATE:
        // Response Header
        response_buffer[0] = (uint8_t)MGMSG_CPLD_UPDATE; ///> \todo Isn't this code redundant due to "respond_to_host" (see cpld_program.c).
        response_buffer[1] = (uint8_t)(MGMSG_CPLD_UPDATE >> 8);
        response_buffer[2] = CPLD_PROG_OK;
        response_buffer[3] = 0x00;
        response_buffer[4] = HOST_ID; /* destination*/
        response_buffer[5] = MOTHERBOARD_ID; /* source*/
        xSemaphoreTake(slave_message->xUSB_Slave_TxSemaphore, portMAX_DELAY);
        xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
        lattice_firmware_update();
        xSemaphoreGive(slave_message->xUSB_Slave_TxSemaphore);
        xSemaphoreGive(xSPI_Semaphore);
        need_to_reply = false;
        setup_and_send_log(slave_message, MOTHERBOARD_ID, LOG_NO_REPEAT,
                SYSTEM_LOG_TYPE, SYSTEM_UPDATE_CPLD_LOG, 0, 0, 0);
        break;

    case MGMSG_SET_HW_REV:
        xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
        set_board_type(
                slave_message->extended_data_buf[0]
                        + (slave_message->extended_data_buf[1] << 8));
        xSemaphoreGive(xSPI_Semaphore);
        setup_and_send_log(slave_message, MOTHERBOARD_ID, LOG_REPEAT, SYSTEM_LOG_TYPE,
                SYSTEM_SET_HW_REV_LOG, 0, 0, 0);
        break;

    case MGMSG_SET_CARD_TYPE:
        set_slot_type(static_cast<slot_nums>(slave_message->extended_data_buf[0]),
                static_cast<slot_types>(slave_message->extended_data_buf[2]
                        + (slave_message->extended_data_buf[3] << 8)));
        setup_and_send_log(slave_message, slave_message->extended_data_buf[0], LOG_REPEAT,
                SYSTEM_LOG_TYPE, SYSTEM_SET_CARD_TYPE_LOG, 0, 0, 0);
        break;

    case MGMSG_REQ_DEVICE:
        response_buffer[0] = (uint8_t)MGMSG_GET_DEVICE;
        response_buffer[1] = (uint8_t)(MGMSG_GET_DEVICE >> 8);


        response_buffer[4] = HOST_ID | 0x80; 	// destination
        response_buffer[5] = MOTHERBOARD_ID;	// source

        /*Chan Ident (slot)*/
        response_buffer[6] = slave_message->param1;
        response_buffer[7] = 0x00;

        ///\bug This is not protected by the slot's lock.
        memcpy(
                response_buffer + 8,
                &slots[slave_message->param1].device.info.signature.device_id,
                sizeof(device_id_t)
        );
        memcpy(
                response_buffer + 8 + sizeof(device_id_t),
                &slots[slave_message->param1].device.info.serial_num,
                        8
                );
        memcpy(
                response_buffer + 16 + sizeof(device_id_t),
                &slots[slave_message->param1].device.info.signature.slot_type,
                sizeof(slot_types)
        );
        memcpy(
                response_buffer + 16 + sizeof(device_signature_t),
                &slots[slave_message->param1].device.info.product_name,
                DEVICE_DETECT_PART_NUMBER_LENGTH
        );
        response_buffer[16 + sizeof(device_signature_t) + DEVICE_DETECT_PART_NUMBER_LENGTH] = '\0';

        ///\bug This is just bad.  Need to have a different way of doing this.
        response_buffer[17 + DEVICE_DETECT_PART_NUMBER_LENGTH + sizeof(device_signature_t)]
                        = slots[slave_message->param1].device._dd_state != 0;

        need_to_reply = true;

        length = 18 + sizeof(device_signature_t) + DEVICE_DETECT_PART_NUMBER_LENGTH;
        response_buffer[2] = (uint8_t)(length-6);
        response_buffer[3] = (uint8_t)((length-6) >> 8);
        break;

    case MGMSG_SET_DEVICE_BOARD:
        {
            const int16_t SLOT = slave_message->extended_data_buf[0] | 
                (slave_message->extended_data_buf[1] << 8);
            xSemaphoreTake(slots[SLOT].xSlot_Mutex, portMAX_DELAY);
            memcpy(&slots[SLOT].save.allowed_device_serial_number, &slave_message->extended_data_buf[2],
                sizeof(slots[SLOT].save.allowed_device_serial_number));
            xSemaphoreGive(slots[SLOT].xSlot_Mutex);

            xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
            slot_save_info_eeprom(static_cast<slot_nums>(SLOT));
            xSemaphoreGive(xSPI_Semaphore);
            device_detect_reset(&slots[SLOT].device);
        }
    break;
    case MGMSG_REQ_DEVICE_BOARD:
        response_buffer[0] = (uint8_t)(MGMSG_GET_DEVICE_BOARD);
        response_buffer[1] = (uint8_t)(MGMSG_GET_DEVICE_BOARD >> 8);
        response_buffer[2] = 10;
        response_buffer[3] = 0;
        response_buffer[4] = HOST_ID | 0x80;
        response_buffer[5] = MOTHERBOARD_ID;

        response_buffer[6] = slave_message->param1;
        response_buffer[7] = 0;

        {
            lock_guard(slots[slave_message->param1].xSlot_Mutex);
            memcpy(&response_buffer[8], &slots[slave_message->param1].save.allowed_device_serial_number,
                sizeof(slots[slave_message->param1].save.allowed_device_serial_number));
        }

        length = 6 + 2 + 8;
        need_to_reply = true;
    break;
    case MGMSG_RESTART_PROCESSOR:
        setup_and_send_log(slave_message, MOTHERBOARD_ID, LOG_REPEAT, SYSTEM_LOG_TYPE,
                SYSTEM_RESTART_PROCESSOR_LOG, 0, 0, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
        restart_board();
        break;

    case MGMSG_ERASE_EEPROM:
        xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
        eeprom_25LC1024_clear_mem();
        xSemaphoreGive(xSPI_Semaphore);
        setup_and_send_log(slave_message, MOTHERBOARD_ID, LOG_REPEAT, SYSTEM_LOG_TYPE,
                SYSTEM_SET_ERASE_EEPROM_LOG, 0, 0, 0);
        // TODO:  Inform EFS that files have been erased.
        break;

    case MGMSG_REQ_CPLD_WR: /*Temp for CPLD write/read register*/
        cpld_write_read(slave_message);
        need_to_reply = false;
        break;

    case MGMSG_TASK_CONTROL: /*Temp for CPLD write/read register*/
        if (slave_message->param1 == 1)
            suspend_all_task();
        else if (slave_message->param1 == 0)
            resume_all_task();
        need_to_reply = false;
        break;

    case MGMSG_BOARD_REQ_STATUSUPDATE:
        response_buffer[0] = (uint8_t)MGMSG_BOARD_GET_STATUSUPDATE;
        response_buffer[1] = (uint8_t)(MGMSG_BOARD_GET_STATUSUPDATE >> 8);
        response_buffer[2] = 7;
        response_buffer[3] = 0x00;
        response_buffer[4] = HOST_ID | 0x80; 	// destination
        response_buffer[5] = MOTHERBOARD_ID;	// source

        /*only temperature_sensor_val | vin_monitor_val | cpu_temp_val | error*/
        memcpy(&response_buffer[6], &board, 7);

        need_to_reply = true;
        length = 6 + response_buffer[2];
        break;

    case MGMSG_MOD_REQ_JOYSTICK_INFO:
        // Header
        response_buffer[0] = (uint8_t)MGMSG_MOD_GET_JOYSTICK_INFO;
        response_buffer[1] = (uint8_t)(MGMSG_MOD_GET_JOYSTICK_INFO >> 8);
        response_buffer[2] = 10; /*Will fill in later once we know how many controls we have*/
        response_buffer[3] = 0x00;
        response_buffer[4] = HOST_ID | 0x80; 	// destination
        response_buffer[5] = MOTHERBOARD_ID;	// source

        /*There are up to 7 devices and 1 hub supported*/
        get_hid_device_info(slave_message->param1 + 1, response_buffer);
        need_to_reply = true;
        length = 6 + response_buffer[2];
        break;

    case MGMSG_MOD_SET_JOYSTICK_MAP_IN:
        {
            auto data = drivers::apt::apt_struct_set
                    <drivers::apt::joystick_map_in>
                    (command_proxy);
            if (data && data->address() <= USB_NUMDEVICES
                     && data->control_number < MAX_NUM_CONTROLS) {
                service::hid_mapping::address_handle::create(data->address())
                        .set_apt(translate(*data), data->control_number);
            }
        }
        need_to_reply = false;
        setup_and_send_log(slave_message, MOTHERBOARD_ID, LOG_REPEAT, SYSTEM_LOG_TYPE,
                SYSTEM_SET_JOYSTICK_MAP_IN_LOG, 0, 0, 0);
        break;

    case MGMSG_MOD_SET_JOYSTICK_MAP_OUT:
        {
            auto data = drivers::apt::apt_struct_set
                    <drivers::apt::joystick_map_out>
                    (command_proxy);
            if (data && data->address() <= USB_NUMDEVICES
                     && data->control_number < MAX_NUM_CONTROLS) {
                service::hid_mapping::address_handle::create(data->address())
                        .set_apt(translate(*data), data->control_number);
            }
        }
        need_to_reply = false;
        setup_and_send_log(slave_message, MOTHERBOARD_ID, LOG_REPEAT, SYSTEM_LOG_TYPE,
                SYSTEM_SET_JOYSTICK_MAP_OUT_LOG, 0, 0, 0);
        break;

    case MGMSG_MOD_REQ_JOYSTICK_MAP_IN:
        {
            auto request = drivers::apt::apt_struct_req
                    <drivers::apt::joystick_map_in>
                    (command_proxy);
            drivers::apt::joystick_map_in::payload_type out;
            if (!request || request->port_number > USB_NUMDEVICES
                   || request->control_number >= MAX_NUM_CONTROLS) {
                memset(&out, 0, sizeof(out));
                out.reverse_direction = 0xFF;
            } else {
                out = translate(service::hid_mapping::address_handle::create(
                        request->address())
                            .get_apt_in(request->control_number));

            }
            out.port_number = request->port_number;
            out.control_number = request->control_number;
            drivers::apt::apt_struct_get<drivers::apt::joystick_map_in>(
                response_builder, out);
            need_to_reply = true;
        }

        // Header
        response_buffer[0] = (uint8_t)MGMSG_MOD_GET_JOYSTICK_MAP_IN;
        response_buffer[1] = (uint8_t)(MGMSG_MOD_GET_JOYSTICK_MAP_IN >> 8);
        response_buffer[2] = drivers::apt::joystick_map_in::payload_type::APT_SIZE;
        response_buffer[3] = 0x00;
        response_buffer[4] = HOST_ID | 0x80; 	// destination
        response_buffer[5] = MOTHERBOARD_ID;	// source
        need_to_reply = true;
        length = 6 + response_buffer[2];
        break;

    case MGMSG_MOD_REQ_JOYSTICK_MAP_OUT:
        {
            auto request = drivers::apt::apt_struct_req
                    <drivers::apt::joystick_map_out>
                    (command_proxy);
            drivers::apt::joystick_map_out::payload_type out;
            if (!request || request->port_number > USB_NUMDEVICES
                   || request->control_number >= MAX_NUM_CONTROLS) {
                memset(&out, 0, sizeof(out));
                out.usage_type = 0xFF;
            } else {
                out = translate(service::hid_mapping::address_handle::create(
                        request->address())
                            .get_apt_out(request->control_number));
            }
            out.port_number = request->port_number;
            out.control_number = request->control_number;
            drivers::apt::apt_struct_get<drivers::apt::joystick_map_out>(
                response_builder, out);
            need_to_reply = true;
        }

        // Header
        response_buffer[0] = (uint8_t)MGMSG_MOD_GET_JOYSTICK_MAP_OUT;
        response_buffer[1] = (uint8_t)(MGMSG_MOD_GET_JOYSTICK_MAP_OUT >> 8);
        response_buffer[2] = drivers::apt::joystick_map_out::payload_type::APT_SIZE;
        response_buffer[3] = 0x00;
        response_buffer[4] = HOST_ID | 0x80; 	// destination
        response_buffer[5] = MOTHERBOARD_ID;	// source

        need_to_reply = true;
        length = 6 + response_buffer[2];
        break;

    case MGMSG_MCM_REQ_JOYSTICK_DATA:
        // Header
        response_buffer[0] = (uint8_t)MGMSG_MCM_GET_JOYSTICK_DATA;
        response_buffer[1] = (uint8_t)(MGMSG_MCM_GET_JOYSTICK_DATA >> 8);
        response_buffer[2] = 4;
        response_buffer[3] = 0x00;
        response_buffer[4] = HOST_ID | 0x80; 	// destination
        response_buffer[5] = MOTHERBOARD_ID;	// source

        if(!usb_device[1].is_hub)
        {
            response_buffer[6] = usb_device[1].hid_data.slot_select;
            response_buffer[7] = NO_SLOT;
            response_buffer[8] = NO_SLOT;
            response_buffer[9] = NO_SLOT;
        }
        else
        {
            if (usb_device[2].enumerated)
                response_buffer[6] = usb_device[2].hid_data.slot_select;
            else
                response_buffer[6] = NO_SLOT;

            if (usb_device[3].enumerated)
                response_buffer[7] = usb_device[3].hid_data.slot_select;
            else
                response_buffer[7] = NO_SLOT;

            if (usb_device[4].enumerated)
                response_buffer[8] = usb_device[4].hid_data.slot_select;
            else
                response_buffer[8] = NO_SLOT;

            if (usb_device[5].enumerated)
                response_buffer[9] = usb_device[5].hid_data.slot_select;
            else
                response_buffer[9] = NO_SLOT;
        }
        need_to_reply = true;
        length = 6 + response_buffer[2];
        break;

    case MGMSG_MOD_SET_SYSTEM_DIM:
        board.dim_bound = slave_message->param1;

        setup_and_send_log(slave_message, MOTHERBOARD_ID, LOG_NO_REPEAT,
                SYSTEM_LOG_TYPE, SYSTEM_SET_DIM_LOG, 0, 0, 0);
        break;

    case MGMSG_MOD_REQ_SYSTEM_DIM:
        // Header
        response_buffer[0] = (uint8_t)MGMSG_MOD_GET_SYSTEM_DIM;
        response_buffer[1] = (uint8_t)(MGMSG_MOD_GET_SYSTEM_DIM >> 8);
        response_buffer[2] = board.dim_bound;
        response_buffer[3] = 0x00;
        response_buffer[4] = HOST_ID; 	// destination
        response_buffer[5] = MOTHERBOARD_ID;	// source

        need_to_reply = true;
        length = 6;
        break;

    case MGMSG_MCM_START_LOG:
        board.send_log_ready = true;
        setup_and_send_log(slave_message, MOTHERBOARD_ID, LOG_REPEAT, SYSTEM_LOG_TYPE,
                SYSTEM_HW_START_LOG, 0, 0, 0);
        break;

    case MGMSG_MCM_SET_ENABLE_LOG:
#if ENABLE_SYSTEM_LOGGING
        board.save.enable_log = slave_message->param1;
        board_save_info_eeprom();
        setup_and_send_log(slave_message, MOTHERBOARD_ID, LOG_REPEAT, SYSTEM_LOG_TYPE,
                SYSTEM_ENABLE_LOG, 0, 0, 0);
#endif
        break;


    case MGMSG_MCM_REQ_ENABLE_LOG:
        // Header
        response_buffer[0] = (uint8_t)MGMSG_MCM_GET_ENABLE_LOG;
        response_buffer[1] = (uint8_t)(MGMSG_MCM_GET_ENABLE_LOG >> 8);
        response_buffer[2] = board.save.enable_log;
        response_buffer[3] = 0x00;
        response_buffer[4] = HOST_ID; 	// destination
        response_buffer[5] = MOTHERBOARD_ID;	// source

        need_to_reply = true;
        length = 6;
        break;

    case MGMSG_MCM_SET_ALLOWED_DEVICES:
            {
                const uint8_t SIZE = (slave_message->ExtendedData_len - 2) - ((slave_message->ExtendedData_len - 2) % sizeof(device_signature_t));
                const slot_nums slot_num = static_cast<slot_nums>(slave_message->extended_data_buf[0]);
                const uint8_t SPACE_LEFT_IN_PAGE = EEPROM_25LC1024_PAGE_SIZE - sizeof(Slot_Save);

                if (SIZE <= SPACE_LEFT_IN_PAGE)
                {
                        xSemaphoreTake(slots[slot_num].xSlot_Mutex, portMAX_DELAY);

                        if (slots[slot_num].p_allowed_devices != NULL){
                                vPortFree(slots[slot_num].p_allowed_devices);
                        }
                        slots[slot_num].p_allowed_devices = static_cast<device_signature_t*>(pvPortMalloc(SIZE));
                        /// \todo Check for malloc failure.

                        memcpy(slots[slot_num].p_allowed_devices, slave_message->extended_data_buf + 2, SIZE);
                        slots[slot_num].save.num_allowed_devices = SIZE / sizeof(device_signature_t);

                        xSemaphoreGive(slots[slot_num].xSlot_Mutex);

                        // Save new set of allowed devices to EEPROM.
                        xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
                        slot_save_info_eeprom(slot_num);
                        xSemaphoreGive(xSPI_Semaphore);

                        // Signal device detection to restart for this slot.
                        device_detect_reset(&slots[slot_num].device);
                }
            }
        break;
        case MGMSG_MCM_REQ_ALLOWED_DEVICES:
            response_buffer[0] = (uint8_t)MGMSG_MCM_GET_ALLOWED_DEVICES;
            response_buffer[1] = (uint8_t)(MGMSG_MCM_GET_ALLOWED_DEVICES >> 8);
            response_buffer[4] = HOST_ID | 0x80;
            response_buffer[5] = MOTHERBOARD_ID;
            response_buffer[6] = slave_message->param1;
            response_buffer[7] = 0x00;
            {
                static const uint16_t MAX_ALLOWED = 98 / (sizeof(device_signature_t));
                uint8_t SIZE = ((slots[slave_message->param1].save.num_allowed_devices < MAX_ALLOWED)
                        ? slots[slave_message->param1].save.num_allowed_devices : MAX_ALLOWED) * sizeof(device_signature_t);
                xSemaphoreTake(slots[slave_message->param1].xSlot_Mutex, portMAX_DELAY);
                memcpy(response_buffer + 8, slots[slave_message->param1].p_allowed_devices, SIZE);
                xSemaphoreGive(slots[slave_message->param1].xSlot_Mutex);

                SIZE += 2;
                response_buffer[2] = SIZE;
                response_buffer[3] = SIZE >> 8;
                length = 6 + SIZE;

                need_to_reply = true;
            }
        break;
        case MGMSG_MCM_SET_DEVICE_DETECTION:
            {
                uint16_t slot;
                memcpy(&slot, slave_message->extended_data_buf, 2);
                // TODO: slot validate
                xSemaphoreTake(slots[slot].xSlot_Mutex, portMAX_DELAY);
                slots[slot].save.allow_device_detection = slave_message->extended_data_buf[2];
                xSemaphoreGive(slots[slot].xSlot_Mutex);

                // Save new set of allowed devices to EEPROM.
                xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
                slot_save_info_eeprom(static_cast<slot_nums>(slot));
                xSemaphoreGive(xSPI_Semaphore);

                // Signal device detection to restart for this slot.
                device_detect_reset(&slots[slot].device);
            }
        break;
        case MGMSG_MCM_REQ_DEVICE_DETECTION:
            response_buffer[0] = (uint8_t)MGMSG_MCM_GET_DEVICE_DETECTION;
            response_buffer[1] = (uint8_t)(MGMSG_MCM_GET_DEVICE_DETECTION >> 8);
            response_buffer[2] = 3;
            response_buffer[3] = 0;
            response_buffer[4] = HOST_ID | 0x80;
            response_buffer[5] = MOTHERBOARD_ID;
            response_buffer[6] = slave_message->param1;
            response_buffer[7] = 0x00;
            xSemaphoreTake(slots[slave_message->param1].xSlot_Mutex, portMAX_DELAY);
            response_buffer[8] = slots[slave_message->param1].save.allow_device_detection;
            xSemaphoreGive(slots[slave_message->param1].xSlot_Mutex);

            length = 9;
            need_to_reply = true;
        break;
        case MGMSG_MCM_SET_SLOT_TITLE:
            {
                uint16_t slot;
                memcpy(&slot, slave_message->extended_data_buf, 2);
                // TODO: Slot validate.
                xSemaphoreTake(slots[slot].xSlot_Mutex, portMAX_DELAY);
                memcpy(&slots[slot].save.user_slot_name,
                       slave_message->extended_data_buf + 2,
                       USER_SLOT_NAME_LENGTH
                );
                xSemaphoreGive(slots[slot].xSlot_Mutex);

                // Save new slot name to EEPROM.
                xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
                slot_save_info_eeprom(static_cast<slot_nums>(slot));
                xSemaphoreGive(xSPI_Semaphore);
            }
        break;
        case MGMSG_MCM_REQ_SLOT_TITLE:
            response_buffer[0] = static_cast<uint8_t>(MGMSG_MCM_GET_SLOT_TITLE);
            response_buffer[1] = static_cast<uint8_t>(MGMSG_MCM_GET_SLOT_TITLE >> 8);
            response_buffer[2] = 2 + USER_SLOT_NAME_LENGTH;
            response_buffer[3] = 0;
            response_buffer[4] = HOST_ID | 0x80;
            response_buffer[5] = MOTHERBOARD_ID;
            {
                uint16_t slot = slave_message->param1 | (static_cast<uint16_t>(slave_message->param2) << 8);
                memcpy(response_buffer + 6, &slot, sizeof(slot));
                lock_guard lg(slots[slot].xSlot_Mutex);
                memcpy(response_buffer + 6 + sizeof(slot), slots[slot].save.user_slot_name, USER_SLOT_NAME_LENGTH);
            }

            length = 24;
            need_to_reply = true;
        break;
        case MGMSG_MOD_REQ_JOYSTICK_CONTROL:
            // Assert(Extra length == 3)
            response_buffer[0] = static_cast<uint8_t>(MGMSG_MOD_GET_JOYSTICK_CONTROL);
            response_buffer[1] = static_cast<uint8_t>(MGMSG_MOD_GET_JOYSTICK_CONTROL >> 8);
            response_buffer[2] = 0;
            response_buffer[3] = 0;
            response_buffer[4] = HOST_ID | 0x80;
            response_buffer[5] = MOTHERBOARD_ID;
            {
                const uint8_t USB_PORT = slave_message->extended_data_buf[0];
                const bool IS_OUT = slave_message->extended_data_buf[1] > 0;
                const uint8_t CONTROL_NUMBER = slave_message->extended_data_buf[2];

                response_buffer[2] += get_hid_control_info(USB_PORT + 1, IS_OUT, CONTROL_NUMBER, response_buffer + 6);
            }

            length = 6 + response_buffer[2];
            need_to_reply = true;
        break;


    // BEGIN        OW APT Commands

    case MGMSG_OW_SET_PROGRAMMING:
        device_detect_change_programming(&slots[slave_message->extended_data_buf[0]].device,
                                         slave_message->extended_data_buf[2],
                                         &callback_ow_response,
                                         (void*)slave_message->extended_data_buf);
        break;
        case MGMSG_OW_REQ_PROGRAMMING:
            device_detect_check_programming(&slots[slave_message->extended_data_buf[0]].device, &callback_ow_response, (void*)slave_message->extended_data_buf);
        break;
        case MGMSG_OW_GET_PROGRAMMING: // This is sent to this slot when the callback function for the SET or REQ commands is called.
            response_buffer[0] = (uint8_t)MGMSG_OW_GET_PROGRAMMING;
            response_buffer[1] = (uint8_t)(MGMSG_OW_GET_PROGRAMMING >> 8);
            response_buffer[2] = 3;
            response_buffer[3] = 0;
            response_buffer[4] = HOST_ID | 0x80;
            response_buffer[5] = MOTHERBOARD_ID;
            response_buffer[6] = slave_message->extended_data_buf[0];
            response_buffer[7] = slave_message->extended_data_buf[1];
            response_buffer[8] = slave_message->extended_data_buf[2];

            length = 9;
            need_to_reply = true;
        break;
        case MGMSG_OW_PROGRAM:
        {
            uint8_t * const p_buffer = static_cast<uint8_t*>(pvPortMalloc(slave_message->ExtendedData_len));
            if (p_buffer == NULL) {
                response_buffer[0] = (uint8_t)MGMSG_OW_REQ_PROGRAMMING;
                response_buffer[1] = (uint8_t)(MGMSG_OW_REQ_PROGRAMMING >> 8);
                response_buffer[2] = 3;
                response_buffer[3] = 0;
                response_buffer[4] = HOST_ID | 0x80;
                response_buffer[5] = MOTHERBOARD_ID;
                response_buffer[6] = slave_message->extended_data_buf[0];
                response_buffer[7] = slave_message->extended_data_buf[1];
                response_buffer[8] = OWPE_PACKET_TOO_LARGE;

                length = 9;
                need_to_reply = true;
            } else {
                memcpy(p_buffer, slave_message->extended_data_buf + 2, slave_message->ExtendedData_len - 2);

                device_detect_handle_ow_programming_packet(
                    &slots[slave_message->extended_data_buf[0]].device,
                    p_buffer,
                    slave_message->ExtendedData_len - 2,
                    &callback_ow_program,
                    (void*)slave_message->extended_data_buf
                );
            }
        }
        break;


    case MGMSG_OW_REQ_PROGRAMMING_SIZE:

            response_buffer[0] = (uint8_t)MGMSG_OW_GET_PROGRAMMING_SIZE;
            response_buffer[1] = (uint8_t)(MGMSG_OW_GET_PROGRAMMING_SIZE >> 8);
            response_buffer[2] = 0x02;
            response_buffer[3] = 0x00;
            response_buffer[4] = HOST_ID | 0x80;
            response_buffer[5] = MOTHERBOARD_ID;
            response_buffer[6] = (uint8_t)(DEVICE_DETECT_PROGRAMMING_BUFFER_SIZE);
            response_buffer[7] = (uint8_t)(DEVICE_DETECT_PROGRAMMING_BUFFER_SIZE >> 8);

            need_to_reply = true;
            length = 8;
            break;

    // END          OW APT Commands
    // BEGIN	EFS APT Commands
    case MGMSG_MCM_EFS_REQ_HWINFO:
    
        response_buffer[0] = static_cast<uint8_t>(MGMSG_MCM_EFS_GET_HWINFO);
        response_buffer[1] = static_cast<uint8_t>(MGMSG_MCM_EFS_GET_HWINFO >> 8);
        response_buffer[2] = 22;
        response_buffer[3] = 0;
        response_buffer[4] = HOST_ID | 0x80;
        response_buffer[5] = MOTHERBOARD_ID;
        {
            const efs::efs_header HEADER = efs::get_header_info();
            const uint16_t MAX_FILES = efs::get_maximum_files();
            const uint16_t FILES_REMAINING = efs::get_free_files();
            const uint16_t PAGES_REMAINING = efs::get_free_pages();
            memcpy(&response_buffer[6], &HEADER, 16);
            memcpy(&response_buffer[22], &MAX_FILES, 2);
            memcpy(&response_buffer[24], &FILES_REMAINING, 2);
            memcpy(&response_buffer[26], &PAGES_REMAINING, 2);
        }

        need_to_reply = true;
        length = 22 + 6;
    break;

    case MGMSG_MCM_EFS_SET_FILEINFO:
        {
            efs::file_identifier_t ident = slave_message->extended_data_buf[0];
            efs::file_attributes_t attr = slave_message->extended_data_buf[1];
            uint16_t page_length;

            memcpy(&page_length, &slave_message->extended_data_buf[2], 2);

            if (page_length == 0) {
                // Deleting an existing file.
                efs::handle handy = efs::get_handle(ident, APT_EFS_TIMEOUT, efs::EXTERNAL);
                if (handy.is_valid())
                {
                    // TODO:  Deletion logging?
                    handy.delete_file();
                }
            } else {
                // Creating a new file.
                // TODO:  Creation logging?
                efs::create_file(ident, page_length, attr, APT_EFS_TIMEOUT);
                efs::add_to_external_cache(ident, APT_EFS_TIMEOUT);
            }
        }
    break;
    case MGMSG_MCM_EFS_REQ_FILEINFO:
        response_buffer[0] = static_cast<uint8_t>(MGMSG_MCM_EFS_GET_FILEINFO);
        response_buffer[1] = static_cast<uint8_t>(MGMSG_MCM_EFS_GET_FILEINFO >> 8);
        response_buffer[2] = 6;
        response_buffer[3] = 0;
        response_buffer[4] = HOST_ID | 0x80;
        response_buffer[5] = MOTHERBOARD_ID;
        {
            const efs::file_identifier_t IDENT = slave_message->extended_data_buf[0];
            const bool EXISTS = efs::does_file_exist(IDENT, APT_EFS_TIMEOUT);
            efs::handle handy = efs::get_handle(IDENT, APT_EFS_TIMEOUT, efs::EXTERNAL);
            const bool OWNED = EXISTS && !handy.is_valid();
            efs::file_attributes_t attr = 0;
            uint16_t page_size = 0;
            if (!OWNED)
            {
                attr = handy.get_attr();
                page_size = handy.get_page_length();
            }

            response_buffer[6] = IDENT;
            response_buffer[7] = (EXISTS) ? 1 : 0;
            response_buffer[8] = (OWNED) ? 1 : 0;
            response_buffer[9] = attr;
            response_buffer[10] = static_cast<uint8_t>(page_size);
            response_buffer[11] = static_cast<uint8_t>(page_size >> 8);
        }


        length = 6 + 6;
        need_to_reply = true;
    break;

    case MGMSG_MCM_EFS_SET_FILEDATA:
    {
        const efs::file_identifier_t IDENT = slave_message->extended_data_buf[0];
        efs::handle file = efs::get_handle(IDENT, APT_EFS_TIMEOUT, efs::EXTERNAL);
        if (file.is_valid())
        {
            uint32_t FILE_ADDR;
            memcpy(&FILE_ADDR, &slave_message->extended_data_buf[1], sizeof(FILE_ADDR));
            const uint16_t DATA_LENGTH = slave_message->ExtendedData_len - 5;
            file.write(FILE_ADDR, &slave_message->extended_data_buf[5], DATA_LENGTH);
        }
    }
    break;
    case MGMSG_MCM_EFS_REQ_FILEDATA:
        response_buffer[0] = static_cast<uint8_t>(MGMSG_MCM_EFS_GET_FILEDATA);
        response_buffer[1] = static_cast<uint8_t>(MGMSG_MCM_EFS_GET_FILEDATA >> 8);
        {
            static constexpr uint16_t DYNAMIC_DATA_CAPACITY = sizeof(response_buffer) - 6 - 1 - 4;
            // Response buffer needs to hold the APT header, File ID, and address before the response.
            const efs::file_identifier_t IDENT = slave_message->extended_data_buf[0];
            uint32_t READ_ADDR;
            uint16_t read_len;
            memcpy(&READ_ADDR, &slave_message->extended_data_buf[1], 4);
            memcpy(&read_len, &slave_message->extended_data_buf[5], 2);
            read_len = std::min(DYNAMIC_DATA_CAPACITY, read_len);

            const efs::handle file = efs::get_handle(IDENT, APT_EFS_TIMEOUT, efs::EXTERNAL);
            uint16_t data_read = 0;
            if (file.is_valid())
            {
                data_read = file.read(READ_ADDR, &response_buffer[6+5], read_len);
            }
            const uint16_t APT_LENGTH = data_read + 5;

            response_buffer[2] = static_cast<uint8_t>(APT_LENGTH);
            response_buffer[3] = static_cast<uint8_t>(APT_LENGTH >> 8);
            response_buffer[4] = HOST_ID | 0x80;
            response_buffer[5] = MOTHERBOARD_ID;
            response_buffer[6] = IDENT;
            memcpy(&response_buffer[7], &READ_ADDR, 4);

            length = 6 + APT_LENGTH;
        }

        need_to_reply = true;
    break;

    
    // END		EFS APT Commands

    case MGMSG_MCM_LUT_SET_LOCK:
        lut_manager::set_lock(slave_message->param1 != 0);

    break;
    case MGMSG_MCM_LUT_REQ_LOCK:
        response_buffer[0] = static_cast<uint8_t>(MGMSG_MCM_LUT_GET_LOCK);
        response_buffer[1] = static_cast<uint8_t>(MGMSG_MCM_LUT_GET_LOCK >> 8);
        response_buffer[2] = lut_manager::get_lock();
        response_buffer[3] = 0;
        response_buffer[4] = HOST_ID;
        response_buffer[5] = MOTHERBOARD_ID;

        need_to_reply = true;
        length = 6;
    break;

    case MGMSG_MCM_REQ_PNPSTATUS:
        response_buffer[0] = (uint8_t)MGMSG_MCM_GET_PNPSTATUS;
        response_buffer[1] = (uint8_t)(MGMSG_MCM_GET_PNPSTATUS >> 8);
        response_buffer[2] = 6;
        response_buffer[3] = 0;
        response_buffer[4] = HOST_ID | 0x80;
        response_buffer[5] = MOTHERBOARD_ID;
        {
            const uint16_t SLOT = slave_message->param1 | (slave_message->param2 << 8);
            memcpy(&response_buffer[6], &SLOT, sizeof(SLOT));
            lock_guard(slots[SLOT].xSlot_Mutex);
            memcpy(&response_buffer[8], &slots[SLOT].pnp_status, sizeof(pnp_status_t));
        }

        need_to_reply = true;
        length = 12;
    break;
    case MGMSG_MOT_SET_EEPROMPARAMS:
        {
            uint16_t channel;
            uint16_t command;
            memcpy(&channel, &slave_message->extended_data_buf[0], sizeof(channel));
            memcpy(&command, &slave_message->extended_data_buf[2], sizeof(command));

            drivers::spi::handle_factory factory;
            switch(command)
            {
            case MGMSG_MOD_SET_JOYSTICK_MAP_IN:
                // Parse the channel as the port number.
                service::hid_mapping::address_handle::create(
                    static_cast<uint8_t>(channel) + 1).store_in_to_eeprom(
                                factory);
            break;
            case MGMSG_MOD_SET_JOYSTICK_MAP_OUT:
                // Parse the channel as the port number.
                service::hid_mapping::address_handle::create(
                    static_cast<uint8_t>(channel) + 1).store_out_to_eeprom(
                                factory);
            default:
            break;
            }
        }
    break;

    default:
        usb_slave_command_not_supported(slave_message);
        break;
    }

    /* If we need a response*/
    if (need_to_reply)
    {
        /* block until we can send the message on the USB slave Tx port*/
        xSemaphoreTake(slave_message->xUSB_Slave_TxSemaphore, portMAX_DELAY);
//		debug_print("%d \r\n",length);

        slave_message->write(response_buffer, length);
        xSemaphoreGive(slave_message->xUSB_Slave_TxSemaphore);
    }
}

static std::size_t hw_info_response(uint8_t p_buffer[RESPONSE_BUFFER_SIZE], const uint16_t get_command)
{
    /* Header*/
    p_buffer[0] = (uint8_t)get_command;
    p_buffer[1] = (uint8_t)(get_command >> 8);
    p_buffer[2] = 0x54;
    p_buffer[3] = 0x00;
    p_buffer[4] = HOST_ID | 0x80; /* destination*/
    p_buffer[5] = MOTHERBOARD_ID; /* source*/

    // MGMSG_HW_GET_INFO serial number, not large enough for us
    for (uint8_t index = 6; index < 10; ++index)
    {
        p_buffer[index] = 0;
    }

    /* 8 bytes of model number*/
    for (int i = 10; i < 18; ++i) { p_buffer[i] = '\0'; }
    // (sbenish)  This has a fixed with, so I'm suppressing any warning from this.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
    strncpy(reinterpret_cast<char*>(&p_buffer[10]), MODEL_NUMBER, 8);
#pragma GCC diagnostic pop

    /* type*/
    p_buffer[18] = 0;
    p_buffer[19] = 0;

    /* 4 bytes of firmware version*/
    p_buffer[20] = FW_PATCH_VER;
    p_buffer[21] = FW_MINOR_VER;
    p_buffer[22] = FW_MAJOR_VER;

    // CPLD Revision
    p_buffer[23] = cpld_revision;
    p_buffer[24] = cpld_revision_minor;

    // USB Serial Number
    for (uint8_t i = 0; i < USB_DEVICE_GET_SERIAL_NAME_LENGTH; ++i)
    {
        p_buffer[25 + i] = USB_DEVICE_GET_SERIAL_NAME_POINTER[i];
    }

    // Maximum payload size
    const uint16_t TMP = RESPONSE_BUFFER_SIZE - APT_COMMAND_SIZE;
    memcpy(
        &p_buffer[25 + USB_DEVICE_GET_SERIAL_NAME_LENGTH],
        &TMP,
        2
    );

    /* fill the rest of Notes and "Empty Space" with 0*/
    for (uint8_t index = 27 + USB_DEVICE_GET_SERIAL_NAME_LENGTH; index < 68; ++index)
    {
        p_buffer[index] = 0;
    }

    // Slot card types
    uint8_t index_temp = 68;
    for (slot_nums slot = SLOT_1; slot < NUMBER_OF_BOARD_SLOTS; slot++)
    {
            memcpy(&p_buffer[index_temp],
                            &slots[slot].save.static_card_type,
                            sizeof(slots[slot].save.static_card_type));

            index_temp += 2;
    }

    /* 2 bytes of hardware version*/
    p_buffer[84] = board_type;
    p_buffer[85] = board_type >> 8;

    /* 2 bytes of modification state of the hardware (not used)*/
    p_buffer[86] = 0;
    p_buffer[87] = 0;

    /* nchs - number of channels*/
    p_buffer[88] = NUMBER_OF_BOARD_SLOTS;
    p_buffer[89] = 0;	// always zero

    return 90;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

bool apt_parse(USB_Slave_Message *slave_message)
{
    bool error = false;

    /*If we are here we now all the data for this command is ready to copy to
     * our message structure*/
    /*Check if packet need to be parsed here.  This will be determined by the destination
     parameter in the ASF command.*/
    if ((slave_message->destination == MOTHERBOARD_ID)
            || (slave_message->destination == MOTHERBOARD_ID_STANDALONE))
    {
        parse(slave_message);
    }
    /*Need to dispatch a message queue to either the uC USB or the FTDI USB*/
    else
    {
        if (service::itc::pipeline_cdc().send(*slave_message, static_cast<asf_destination_ids>(slave_message->destination)) == 0) {
            if (slave_message->destination >= SLOT_1_ID && slave_message->destination <= SLOT_7_ID) {
                if (slave_message->ftdi_flag) {
                    debug_print("ERROR: The USB FTDI failed to post the message to %d./r/n",
                        slave_message->destination - SLOT_1_ID);
                } else {
                    debug_print("ERROR: The USB Slave failed to post the message to %d./r/n",
                        slave_message->destination - SLOT_1_ID);
                }
            } else {
                if (slave_message->ftdi_flag) {
                    debug_print("ERROR: USB FTDI error, buffer cleared. dest %d/r/n", slave_message->destination);
                } else {
                    debug_print("ERROR: USB Slave error, buffer cleared./r/n");
                }
            }
        }
    }
    return error;
}

void usb_slave_command_not_supported(USB_Slave_Message *slave_message)
{
    debug_print("ERROR: %H APT command not implemented.\r\n",
            slave_message->ucMessageID);
}

static Hid_Mapping_control_in translate(const drivers::apt::joystick_map_in::payload_type& value) {
    return {
        .idVendor = value.permitted_vendor_id,
        .idProduct = value.permitted_product_id,
        .modify_control_port = value.target_port_numbers_bitset,
        .modify_control_ctl_num = value.target_control_numbers_bitset,
        .destination_slot = value.destination_slots_bitset,
        .destination_bit = value.destination_channels_bitset,
        .destination_port = value.destination_ports_bitset,
        .destination_virtual = value.destination_virtual_bitset,
        .speed_modifier = value.speed_modifier,
        .reverse_dir = value.reverse_direction,
        .dead_band = value.dead_band,
        .mode = static_cast<Control_Modes>(value.control_mode),
        .control_disable = value.control_disabled,
    };
}
static drivers::apt::joystick_map_in::payload_type translate(const Hid_Mapping_control_in& value) {
    return {
        .control_mode = static_cast<uint32_t>(value.mode),
        .permitted_vendor_id = value.idVendor,
        .permitted_product_id = value.idProduct,
        .target_control_numbers_bitset = value.modify_control_ctl_num,
        .target_port_numbers_bitset = value.modify_control_port,
        .destination_slots_bitset = value.destination_slot,
        .destination_channels_bitset = value.destination_bit,
        .destination_ports_bitset = value.destination_port,
        .destination_virtual_bitset = value.destination_virtual,
        .speed_modifier = value.speed_modifier,
        .dead_band = value.dead_band,
        .reverse_direction = value.reverse_dir != 0,
        .control_disabled = value.control_disable,
    };
}

static Hid_Mapping_control_out translate(const drivers::apt::joystick_map_out::payload_type& value) {
    return {
        .type = value.usage_type,
        .idVendor = value.permitted_vendor_id,
        .idProduct = value.permitted_product_id,
        .mode = static_cast<Led_modes>(value.led_mode),
        .color_1_id = static_cast<Led_color_id>(value.color_ids[0]),
        .color_2_id = static_cast<Led_color_id>(value.color_ids[1]),
        .color_3_id = static_cast<Led_color_id>(value.color_ids[2]),
        .source_slots = value.source_slot_bitset,
        .source_bit = value.source_channel_bitset,
        .source_port = value.source_port_bitset,
        .source_virtual = value.source_virtual_bitset,
    };
}
static drivers::apt::joystick_map_out::payload_type translate(const Hid_Mapping_control_out& value) {
    return {
        .permitted_vendor_id = value.idVendor,
        .permitted_product_id = value.idProduct,
        .color_ids = {static_cast<uint8_t>(value.color_1_id),
                      static_cast<uint8_t>(value.color_2_id),
                      static_cast<uint8_t>(value.color_3_id)},
        .usage_type = value.type,
        .led_mode = static_cast<uint8_t>(value.mode),
        .source_slot_bitset = value.source_slots,
        .source_channel_bitset = value.source_bit,
        .source_port_bitset = value.source_port,
        .source_virtual_bitset = value.source_virtual,
    };
}
