#include <concepts>
#include <cstdint>
#include <cstring>
#include <optional>

// #include "Debugging.h"
#include "Debugging.h"
#include "compiler.h"
#include "hid-mapping.hh"
#include "hid.h"
#include "hid_in.h"
#include "hid_report.h"
#include "itc-service.hh"
#include "lock_guard.hh"
#include "max3421e_usb.h"
#include "slots.h"
#include "usb_device.hh"
#include "user_spi.h"
/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Macros
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/

/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
/// \brief Extracts the value of a control from the device' data.
static int16_t extract_control_value(const UsbDevice& device, uint8_t ctrl_no);

/**
 * Extracts the value of a control from the data buffer.
 * \pre data_buffer has sufficient size for the ctrl's bit and byte offsets.
 * \param[in] ctrl        The descriptor of the HID IN control that needs
 *                        parsing.
 * \param[in] data_buffer The buffer that contains the IN report data.
 */
static int16_t extract_control_value(const Hid_Control_info& ctrl,
                                     const uint8_t* data_buffer);

/// \brief Applies any supported HID control item type bits to the value,
///        then returns the modified value.
static int16_t apply_item_bits(Hid_Control_info& info, int16_t value);

/// \brief Normalizes the value to be a tristate of -1, 0, or 1.
static int16_t normalize_dial(Hid_Control_info& info, int16_t value);

/// \brief Normalizes the value to the range [-1, 1] where 0 is the mean
///        of the control's logical min and max values.
static float normalize_axis(Hid_Control_info& info,
                            const Hid_Mapping_control_in& mapping,
                            int16_t value);

void apply_overload(service::hid_mapping::address_handle& handle, uint16_t dest,
                    uint16_t src, bool toggle, const auto& type) {
    if (toggle && handle.get_overload(type, dest) == &handle.get_apt_in(src)) {
        handle.clear_overload(type, dest);
    } else {
        handle.set_overload(type, dest, src);
    }
}

/**
 * Template method that applies the template_ctrl
 * to its self-defined targets using the mutor
 * function.
 */
static void modify_controls(
    service::hid_mapping::address_handle& existing_handle, uint16_t src,
    auto&& callback) {
    const Hid_Mapping_control_in& template_ctrl =
        existing_handle.get_apt_in(src);
    for (uint8_t port = 0; port < 8 * sizeof(template_ctrl.modify_control_port);
         ++port) {
        // Skip if the bit is not set.
        if ((template_ctrl.modify_control_port & (1 << port)) == 0) {
            continue;
        }
        const uint8_t DEST_ADDR = port + 1;

        // (sbenish) Not supporting foreign address modifications at the moment.
        if (DEST_ADDR != existing_handle.address()) {
            continue;
        }

        for (std::size_t ctrl = 0;
             ctrl < 8 * sizeof(template_ctrl.modify_control_ctl_num); ++ctrl) {
            // Skip if the bit is not set.
            if ((template_ctrl.modify_control_ctl_num & (1 << ctrl)) == 0) {
                continue;
            }

            callback(existing_handle, ctrl, src);
        }
    }
}

/*****************************************************************************
 * Static Data
 *****************************************************************************/

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
void drivers::usb_device::service_in_report(
    UsbDevice& device, service::hid_mapping::address_handle& mappings) {
    // -1 represents that the hid_in_ep was not found.
    if (device.hid_data.hid_in_ep == -1) {
        return;
    }

    const uint8_t IN_CONTROL_COUNT = device.hid_data.num_of_input_controls;

    if (!device.initialized) {
        device.hid_data.num_of_input_controls_initialized = 0;
    }

    {
        auto _ = lock_guard(xSPI_Semaphore);
        usb_read(mappings.address(), device.hid_data.hid_in_ep,
                 device.hid_data.num_of_input_bytes, device.hid_data.hid_buf,
                 nullptr, 0);
    }

    for (uint8_t ctrl_no = 0;
         ctrl_no < std::min<uint8_t>(IN_CONTROL_COUNT, mappings.in_map_size());
         ++ctrl_no) {
        const Hid_Mapping_control_in& MAPPING = mappings.in_map(ctrl_no);
        const uint16_t DEVICE_VID             = device.dev_descriptor.idVendor;
        const uint16_t DEVICE_PID             = device.dev_descriptor.idProduct;

        const uint16_t CONTROL_VID = MAPPING.idVendor;
        const uint16_t CONTROL_PID = MAPPING.idProduct;

        if ((CONTROL_VID != 0 || CONTROL_PID != 0) &&
            (DEVICE_VID != CONTROL_VID || DEVICE_PID != CONTROL_PID)) {
            // VID-PID verification failed -> Skip parsing this control.
            continue;
        }

        const uint8_t BYTE_POS =
            device.hid_data.hid_in_controls[ctrl_no].byte_position;

        const bool BYTE_CHANGED = device.hid_data.hid_buf[BYTE_POS] !=
                                      device.hid_data.hid_prev_buf[BYTE_POS] ||
                                  !device.initialized;

        if (!BYTE_CHANGED) {
            continue;
        }

        if (device.hid_data.num_of_input_controls_initialized <
            device.hid_data.num_of_input_controls) {
            ++device.hid_data.num_of_input_controls_initialized;
        }

        if (MAPPING.control_disable) {
            continue;
        }

        const int16_t VALUE = extract_control_value(device, ctrl_no);
        service::itc::message::hid_in_message message{
            .mode         = MAPPING.mode,
            .control_data = 0,
            .usage_page =
                HID_get_usage_page(&device.hid_data.hid_in_controls[ctrl_no]),
            .usage_id =
                HID_get_usage_id(&device.hid_data.hid_in_controls[ctrl_no]),
            .address                    = mappings.address(),
            .destination_channel_bitset = MAPPING.destination_bit,
            .destination_virtual_bitset = MAPPING.destination_virtual,
        };

        static constexpr int8_t STATUS_NO_DISPATCH = 0;
        static constexpr int8_t STATUS_DISPATCH    = 1;
        static constexpr int8_t STATUS_ERROR       = -1;

        auto hid_usage_page_button =
            [](service::itc::message::hid_in_message& message, int16_t value,
               service::hid_mapping::address_handle& mappings,
               uint16_t ctrl) -> int8_t {
            hid_report_print("Button %d: (%d)", ctrl_no, value);
            message.control_data = static_cast<float>(value);

            switch (message.mode) {
            case CTL_BTN_SELECT:
            case CTL_BTN_SELECT_TOGGLE:
                // Applies an IN mapping template to the target controls.
                if (message.control_data > 0) {
                    em_stop(EM_STOP_ALL);

                    modify_controls(
                        mappings, ctrl,
                        [TOGGLE = message.mode == CTL_BTN_SELECT_TOGGLE](
                            service::hid_mapping::address_handle& handle,
                            uint16_t dest, uint16_t src) {
                            const bool SELECT_SAME =
                                handle.get_overload(
                                    service::hid_mapping::address_handle::
                                        selection_overload,
                                    dest) == &handle.get_apt_in(src);
                            const bool SPEED_SAME =
                                handle.get_overload(
                                    service::hid_mapping::address_handle::
                                        speed_overload,
                                    dest) == &handle.get_apt_in(src);
                            if (TOGGLE && SPEED_SAME && SELECT_SAME) {
                                handle.clear_overload(
                                    service::hid_mapping::address_handle::
                                        selection_overload,
                                    dest);
                                handle.clear_overload(
                                    service::hid_mapping::address_handle::
                                        speed_overload,
                                    dest);
                            } else {
                                handle.set_overload(
                                    service::hid_mapping::address_handle::
                                        selection_overload,
                                    dest, src);
                                handle.set_overload(
                                    service::hid_mapping::address_handle::
                                        speed_overload,
                                    dest, src);
                            }
                        });
                }
                break;
            case CTL_BTN_SPEED:
                em_stop(EM_STOP_ALL);

                modify_controls(
                    mappings, ctrl,
                    [FLAG_RESET_SET = message.control_data != 0](
                        service::hid_mapping::address_handle& handle,
                        uint16_t dest, uint16_t src) {
                        if (FLAG_RESET_SET) {
                            handle.set_overload(
                                service::hid_mapping::address_handle::
                                    speed_overload,
                                dest, src);
                        } else {
                            handle.clear_overload(
                                service::hid_mapping::address_handle::
                                    speed_overload,
                                dest);
                        }
                    });
                break;

            case CTL_BTN_SPEED_TOGGLE:
                // Applies an IN mapping template to the target controls.
                if (message.control_data > 0) {
                    em_stop(EM_STOP_ALL);

                    modify_controls(
                        mappings, ctrl,
                        [](service::hid_mapping::address_handle& handle,
                           uint16_t dest, uint16_t src) {
                            apply_overload(handle, dest, src, true,
                                           service::hid_mapping::
                                               address_handle::speed_overload);
                        });
                }
                break;
            case CTL_BTN_DISABLE_MOD:
            case CTL_BTN_TOGGLE_DISABLE_MOD:
                // Disables the control of the target(s).
                if (message.control_data > 0) {
                    em_stop(EM_STOP_ALL);

                    modify_controls(
                        mappings, ctrl,
                        [TOGGLE = message.mode == CTL_BTN_TOGGLE_DISABLE_MOD](
                            service::hid_mapping::address_handle& handle,
                            uint16_t dest, uint16_t src) {
                            const auto& type = service::hid_mapping::
                                address_handle::enable_overload;
                            if (TOGGLE &&
                                handle.get_overload(type, dest) != nullptr) {
                                handle.clear_overload(type, dest);
                            } else {
                                handle.set_overload(type, dest, src);
                            }
                        });
                }
                break;
            // Needs to dispatch a HID IN message.
            case CTL_BTN_TOGGLE_DISABLE_DEST:
            case CTL_BTN_DISABLE_DEST:
            case CTL_BTN_POS_TOGGLE:
            case CTL_BTN_POS_1:
            case CTL_BTN_POS_2:
            case CTL_BTN_POS_3:
            case CTL_BTN_POS_4:
            case CTL_BTN_POS_5:
            case CTL_BTN_POS_6:
            case CTL_BTN_POS_7:
            case CTL_BTN_POS_8:
            case CTL_BTN_POS_9:
            case CTL_BTN_POS_10:
            case CTL_BTN_JOG_CW:
            case CTL_BTN_JOG_CCW:
            case CTL_BTN_HOME:
                return message.control_data != 0 ? STATUS_DISPATCH
                                                 : STATUS_NO_DISPATCH;
                break;
            default:
                return STATUS_ERROR;
                break;
            }

            return STATUS_NO_DISPATCH;
        };

        int8_t status = STATUS_ERROR;
        switch (message.usage_page) {
        case HID_USAGE_PAGE_BUTTON:
            status = hid_usage_page_button(message, VALUE, mappings, ctrl_no);
            break;
        case HID_USAGE_PAGE_GENERIC_DESKTOP:
            switch (message.usage_id) {
            case HID_USAGE_X:
            case HID_USAGE_Y:
            case HID_USAGE_Z:
            case HID_USAGE_RX:
            case HID_USAGE_RY:
            case HID_USAGE_RZ:
            case HID_USAGE_DIAL:
            case HID_USAGE_WHEEL:
                if (message.mode != CTL_AXIS ||
                    device.hid_data.num_of_input_controls_initialized <
                        device.hid_data.num_of_input_controls) {
                    break;
                }
                message.control_data =
                    (message.usage_id == HID_USAGE_DIAL
                         ? normalize_dial(
                               device.hid_data.hid_in_controls[ctrl_no], VALUE)
                         : normalize_axis(
                               device.hid_data.hid_in_controls[ctrl_no],
                               MAPPING, VALUE)) *
                    (MAPPING.reverse_dir == 0 ? 1 : -1);

                status = STATUS_DISPATCH;
                break;
            default:
                status = STATUS_ERROR;
                break;
            }
            break;
        }

        switch (status) {
        case STATUS_NO_DISPATCH:
            break;
        case STATUS_DISPATCH:
            service::itc::pipeline_hid_in().send_if(
                message,
                [bitset = MAPPING.destination_slot](const slot_nums& slot) {
                    return ((0x1 << (uint8_t)slot) & bitset) != 0;
                });
            break;
        case STATUS_ERROR:
        default:
            error_print("ERROR: HID control (%d, %d) not supported",
                        message.usage_page, message.usage_id);
            break;
        }
    }

    // Move the current data into the previous data for the next loop.
    memcpy(device.hid_data.hid_prev_buf, device.hid_data.hid_buf,
           sizeof(device.hid_data.hid_buf));

    device.initialized = true;
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
static int16_t extract_control_value(const UsbDevice& device, uint8_t ctrl_no) {
    return extract_control_value(device.hid_data.hid_in_controls[ctrl_no],
                                 device.hid_data.hid_buf);
}

static int16_t extract_control_value(const Hid_Control_info& ctrl,
                                     const uint8_t* data_buffer) {
    uint8_t byte_bit_position = 0;
    uint8_t mask              = 0;
    int16_t value             = 0;

    /*Get the position to read from the buffer*/
    const uint8_t control_byte_position = ctrl.byte_position;

    switch (ctrl.report_size) {
    case 1: /* 1 bit for buttons*/

        /*Figure out the mask for the control.  Mask = 1 << (bit_position -
         * (byte_position * 8))*/
        byte_bit_position = (ctrl.bit_position - (ctrl.byte_position * 8));
        mask              = 1 << byte_bit_position;
        value = ((data_buffer[control_byte_position] & mask) != 0) ? 1 : 0;
        break;

    case 8: /* 8 bits*/
        // check if the 8 bit value is signed or unsigned
        if (ctrl.logical_min < 0)
            value = (int8_t)data_buffer[control_byte_position];
        else
            value = (uint8_t)data_buffer[control_byte_position];
        break;

    case 16: /* 16 bits*/
        // assume tha the 16 bit value is signed
        value = data_buffer[control_byte_position] +
                (data_buffer[control_byte_position + 1] << 8);
        break;

    default:
        break;
    }

    return value;
}

static int16_t apply_item_bits(Hid_Control_info& info, int16_t value) {
    if (!Tst_bits(info.item_bits, ITEM_ABS_REL) &&
        Tst_bits(info.item_bits, ITEM_PREF_NOPREF)) {
        // Calculate the delta

        if (info.first_time) {
            info.first_time = false;
            info.prev_value = value;
            value           = 0;
        } else {
            const int16_t delta = value - info.prev_value;
            info.prev_value     = value;
            value               = delta;
        }
    }

    if (Tst_bits(info.item_bits, ITEM_WRAP_NOWRAP)) {
        // Calculate the wrap-around.

        int16_t abs_delta = std::abs(value);

        // If the delta is greater than the max value /2, then it
        // must have wrapped around.  Therefore, the delta needs
        // to follow a different calculation.
        if (abs_delta > info.logical_max / 2) {
            abs_delta = info.logical_max - abs_delta;

            value = abs_delta * (abs_delta < 0 ? -1 : 1);
        }
    }

    return value;
}
static int16_t normalize_dial(Hid_Control_info& info, int16_t value) {
    value = apply_item_bits(info, value);

    if (value == 0) {
        return 0;
    } else if (value < 0) {
        return -1;
    } else {
        return 1;
    }
}

static float normalize_axis(Hid_Control_info& info,
                            const Hid_Mapping_control_in& mapping,
                            int16_t value) {
    const uint16_t DEADBAND =
        mapping.dead_band + 1;  // minimum of 1 for rounding.

    value = apply_item_bits(info, value);

    const int16_t Xn1 = info.logical_min;
    const int16_t X0  = (info.logical_min + info.logical_max) / 2;
    const int16_t X1  = info.logical_max;

    // High speed controls the maximum speed coefficient (A)
    const float A = (1 + (float)mapping.speed_modifier) / 256;

    // Linear function terms
    float m;
    float b;

    if (value > X0 + DEADBAND) {
        m = 1.0 / (X1 - X0 - DEADBAND);
        b = -m * (X0 + DEADBAND);
    } else if (value < X0 - DEADBAND) {
        m = 1.0 / (X0 - Xn1 - DEADBAND);
        b = m * (DEADBAND - X0);
    } else {
        m = 0;
        b = 0;
    }

    const float rt = A * (m * value + b);

    hid_report_print("Axis: (%d), Normalized: (%d)", value,
                     (int16_t)(rt * 1000));

    return rt;
}
