/**
 * @file hid.c
 *
 * @brief Functions for HID devices connected to root port or any of the ports
 * on a HUB.
 *
 */

#include <25lc1024.h>
#include <asf.h>
#include <hid.h>
#include <string.h>

#include "usb_device.h"


/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void get_hid_device_info(uint8_t address, uint8_t* response_buffer) {
    uint8_t i = 6;

    /*Port*/
    response_buffer[i++] = address - 1;

    /*Root port VID (response_buffer[6])*/
    response_buffer[i++] = usb_device[address].dev_descriptor.idVendor;
    response_buffer[i++] = usb_device[address].dev_descriptor.idVendor >> 8;

    /*Root port PID (response_buffer[8])*/
    response_buffer[i++] = usb_device[address].dev_descriptor.idProduct;
    response_buffer[i++] = usb_device[address].dev_descriptor.idProduct >> 8;

    /*speed and hub flags (9)*/
    response_buffer[i++] =
        (usb_device[address].lowspeed << 1) + usb_device[address].is_hub;

    if (usb_device[address].is_hub) {
        /*Number of ports (10)*/
        response_buffer[i++] = usb_device[address].hub.bNumberOfPorts;
    } else {
        /*Number of input controls*/
        response_buffer[i++] =
            usb_device[address].hid_data.num_of_input_controls;

        /*Number of output controls*/
        response_buffer[i++] =
            usb_device[address].hid_data.num_of_output_controls;
    }

    /*Fill in the length of the response*/
    response_buffer[2] = i;
}

int get_hid_control_info(uint8_t address, bool isOut, uint8_t control_index,
                         uint8_t* response_buffer) {
    response_buffer[0] = address;
    response_buffer[1] = isOut;
    response_buffer[2] = control_index;
    // 3:   Usage Page
    // 4:   Usage Id

    uint16_t USAGE_PAGE;
    uint16_t USAGE_ID;
    if (address > USB_NUMDEVICES ||
        (isOut &&
         control_index > usb_device[address].hid_data.num_of_output_controls) ||
        (!isOut &&
         control_index > usb_device[address].hid_data.num_of_input_controls)) {
        USAGE_PAGE = 0;
        USAGE_ID   = 0;
    } else if (isOut) {
        // Output control
        USAGE_PAGE = HID_get_usage_page(
            &usb_device[address].hid_data.hid_out_controls[control_index]);
        USAGE_ID = HID_get_usage_id(
            &usb_device[address].hid_data.hid_out_controls[control_index]);
    } else {
        // Input control
        USAGE_PAGE = HID_get_usage_page(
            &usb_device[address].hid_data.hid_in_controls[control_index]);
        USAGE_ID = HID_get_usage_id(
            &usb_device[address].hid_data.hid_in_controls[control_index]);
    }

    memcpy(&response_buffer[3], &USAGE_PAGE, sizeof(USAGE_PAGE));
    memcpy(&response_buffer[5], &USAGE_ID, sizeof(USAGE_ID));

    return 7;
}
