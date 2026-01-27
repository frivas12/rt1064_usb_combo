#include "hid_pid_fixes.h"

#include <asf.h>

#include "hid_report.h"
#include "Debugging.h"
#include "hid.h"
#include "max3421e_usb.h"
#include "string.h"
#include "usb_device.h"
#include "usb_host.h"

void fix_hid_report_parsing_errors(uint8_t address) {
    // space navigoator byte alignment doesn't work with our parsing scheme
    if (usb_device[address].dev_descriptor.idVendor == 0x046d &&
        usb_device[address].dev_descriptor.idProduct == 0xc626) {
        usb_device[address].hid_data.hid_in_controls[0].byte_position = 1;
        usb_device[address].hid_data.hid_in_controls[1].byte_position = 3;
        usb_device[address].hid_data.hid_in_controls[2].byte_position = 5;
        usb_device[address].hid_data.hid_in_controls[3].byte_position = 8;
        usb_device[address].hid_data.hid_in_controls[4].byte_position = 10;
        usb_device[address].hid_data.hid_in_controls[5].byte_position = 12;
        usb_device[address].hid_data.num_of_input_bytes               = 14;
    }

    // if shuttle express set max wheel value to 7
    // shuttle express wheel and dial should both be absolute, HID 1.11
    // specifies (pg30) relative (indicating the change in value from the last
    // report)
    if (usb_device[address].dev_descriptor.idVendor == 0x0b33 &&
        usb_device[address].dev_descriptor.idProduct == 0x0020) {
        usb_device[address].hid_data.hid_in_controls[0].logical_min = 0xfff9;
        usb_device[address].hid_data.hid_in_controls[0].logical_max = 0x0007;
        usb_device[address].hid_data.hid_in_controls[0].item_bits =
            0x02;  // it's absolute, not rel
        usb_device[address].hid_data.hid_in_controls[1].item_bits =
            0x2a;  // it's absolute, not rel

        // The buttons data comes in a bit position 28 but the report states 24
        // so we need to shift this PR
        usb_device[address].hid_data.hid_in_controls[2].bit_position = 28;
        usb_device[address].hid_data.hid_in_controls[3].bit_position = 29;
        usb_device[address].hid_data.hid_in_controls[4].bit_position = 30;
        usb_device[address].hid_data.hid_in_controls[5].bit_position = 31;

        // Only for the shuttlExpress joystick
        // Set the slot_select to slot 1 X axis
        usb_device[address].hid_data.slot_select = 1;
    }

    if (usb_device[address].dev_descriptor.idVendor == 0x1313 &&
        usb_device[address].dev_descriptor.idProduct == 0x2005) {
        usb_device[address].hid_data.hid_in_controls[0].item_bits =
            0x26;  // it's relative not absolute
        usb_device[address].hid_data.hid_in_controls[1].item_bits =
            0x26;  // it's relative not absolute
        usb_device[address].hid_data.hid_in_controls[2].item_bits =
            0x26;  // it's relative not absolute
    }

    // Thorlabs Powermate
    if (usb_device[address].dev_descriptor.idVendor == 0x1313 &&
        usb_device[address].dev_descriptor.idProduct == 0x2010) {
        usb_device[address].hid_data.hid_out_controls[0].usage = 0x00080000;
    }
}
