#include <algorithm>
#include <bitset>
#include <cstring>
#include <initializer_list>

#include "Debugging.h"
#include "FreeRTOSConfig.h"
#include "board.h"
#include "hid-mapping.hh"
#include "hid.h"
#include "hid_out.h"
#include "hid_report.h"
#include "itc-service.hh"
#include "lock_guard.hh"
#include "max3421e_usb.h"
#include "slot_nums.h"
#include "slots.h"
#include "spi-transfer-handle.hh"
#include "supervisor.h"
#include "task.h"
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
struct color_data {
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    constexpr bool operator==(const color_data&) const = default;

    constexpr color_data& operator*=(uint8_t scale) {
        red   = (red * scale) / 255;
        green = (green * scale) / 255;
        blue  = (blue * scale) / 255;
        return *this;
    }

    /**
     * Create a color data applied with the linear scale.
     * \param[in] scale A 1/255 floating-point value.
     */
    constexpr color_data operator*(uint8_t scale) {
        color_data copy = *this;
        copy *= scale;
        return copy;
    }

    /// \brief Sets the color bit if any color channel is non-zero.
    constexpr bool as_monochrome_bit() const {
        return red != 0 || green != 0 || blue != 0;
    }

    /// \brief Selects the largest channel's value as the monochrome brightness.
    constexpr uint8_t as_monochrome_max() const {
        return std::max({red, green, blue});
    }

    /// \brief Selects the average of the channels to determine the monochrome
    /// value.
    constexpr uint8_t as_monochrome_average() const {
        return static_cast<uint8_t>((red + green + blue) / 3);
    }
};

using color_serializer = void (*)(uint8_t[3 * MAX_NUM_CONTROLS], color_data);

/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
static void encode_color(uint8_t serialized[3 * MAX_NUM_CONTROLS],
                         const Hid_Control_info& control, color_data color);
static void encode_color_monochrome_1bit(uint8_t buffer[3 * MAX_NUM_CONTROLS],
                                         const Hid_Control_info& control,
                                         color_data color);
static void encode_color_monochrome_8bit(uint8_t buffer[3 * MAX_NUM_CONTROLS],
                                         const Hid_Control_info& control,
                                         color_data color);
static void encode_color_RG_16bit(uint8_t buffer[3 * MAX_NUM_CONTROLS],
                                  const Hid_Control_info& control,
                                  color_data color);
static void encode_color_RGB_24bit(uint8_t buffer[3 * MAX_NUM_CONTROLS],
                                   const Hid_Control_info& control,
                                   color_data color);
static color_data color_id_to_data(Led_color_id color);

static bool check_if_slot_source(const Hid_Mapping_control_out& mapping,
                                 slot_nums slot);

static void apply_slot_brightness(color_data& inout, uint8_t board_dim,
                                  const Hid_Mapping_control_out& mapping);

static Led_color_id use_stage_coloring(
    const drivers::usb_device::out_report_state::slot_state& slot_state,
    const Hid_Mapping_control_in& in_mapping,
    const Hid_Mapping_control_out& out_mapping, bool invert, bool& blink_fade);

static Led_color_id use_saved_position_coloring(
    const Hid_Mapping_control_out& out, bool on_position);
/*****************************************************************************
 * Static Data
 *****************************************************************************/
static constexpr color_data COLOR_OFF = color_data{0, 0, 0};

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
drivers::usb_device::out_report_state::out_report_state(uint8_t address)
    : resend_cntr(0) {
    memset(mapping_colors, LED_ID_OFF, sizeof(mapping_colors));
}

void drivers::usb_device::service_out_report(
    UsbDevice& device,
    service::itc::queue_handle<service::itc::message::hid_out_message>& queue,
    service::hid_mapping::address_handle& mappings, out_report_state& state) {
    configASSERT(device.hid_data.hid_out_ep != -1);

    // Marking these as volatile to guarantee that these static values
    // are read at the start of the function and are not reordered.
    const volatile bool SAMPLE_GLOBAL_500MS_TICK = global_500ms_tick;
    const volatile uint8_t SAMPLE_FADE_VAL       = fade_val;
    const volatile uint8_t SAMPLE_BOARD_DIM      = board.dim_bound;

    uint8_t LAST_SERIALIZED[sizeof(state.last_out_report)];
    memcpy(LAST_SERIALIZED, state.last_out_report, sizeof(LAST_SERIALIZED));

    bool limit_shade[MAX_NUM_CONTROLS] = {false};

    // Decode the messages to determine the colors of the LEDs.
    // Move state update messages into thread-local memory.
    for (service::itc::message::hid_out_message message;
         queue.try_pop(message, 0);) {
        if (message.slot >= NUMBER_OF_BOARD_SLOTS) {
            continue;
        }

        auto& slot_state                    = state.slot_states[message.slot];
        slot_state.on_saved_position_bitset = message.on_stored_position_bitset;
        slot_state.is_moving(message.is_moving);
        slot_state.on_limit(message.on_limit);
        slot_state.is_enabled(message.is_enabled);
    }

    // Find any DISABLE_ALL controls and use the state information.

    for (uint8_t control_num = 0;
         control_num < device.hid_data.num_of_output_controls; ++control_num) {
        // Verification of control VID, PID (0, 0 to disable verification).
        const uint16_t DEVICE_VID = device.dev_descriptor.idVendor;
        const uint16_t DEVICE_PID = device.dev_descriptor.idProduct;

        const Hid_Control_info& CONTROL =
            device.hid_data.hid_out_controls[control_num];
        const Hid_Mapping_control_out& MAPPING = mappings.out_map(control_num);

        const uint16_t CONTROL_VID = mappings.out_map(control_num).idVendor;
        const uint16_t CONTROL_PID = mappings.out_map(control_num).idProduct;

        const bool VID_PID_VALIDATION_FAILED =
            (CONTROL_VID != 0 || CONTROL_PID != 0) &&
            (DEVICE_VID != CONTROL_VID || DEVICE_PID != CONTROL_PID);

        if (VID_PID_VALIDATION_FAILED) {
            continue;
        }

        Led_color_id color = LED_ID_OFF;

        if (MAPPING.type == HID_USAGE_PAGE_LED) {
            const uint16_t USAGE_PAGE = HID_get_usage_page(&CONTROL);
            const uint16_t USAGE_ID   = HID_get_usage_id(&CONTROL);

            switch (USAGE_PAGE) {
            case HID_USAGE_PAGE_LED:
                /*If the map is for a disable all, store the control number
                 * to use after we cycle though all the controls to update
                 * the control out "disable all" led*/
                if (MAPPING.mode == LED_DISABLE_ALL) {
                    bool all_disabled = true;

                    /*Check slot sources enabled then check each slot */
                    for (uint8_t slot = 0; slot < NUMBER_OF_BOARD_SLOTS;
                         ++slot) {
                        if (check_if_slot_source(MAPPING, (slot_nums)slot) &&
                            state.slot_states[slot].is_enabled()) {
                            all_disabled = false;
                        }
                    }

                    if (all_disabled) {
                        color = MAPPING.color_1_id;
                    } else {
                        color = MAPPING.color_2_id;
                    }
                } else if (MAPPING.mode == LED_SPEED_SWITCH ||
                           MAPPING.mode == LED_TOGGLE) {
                    const bool using_original_mapping =
                        MAPPING.source_slots ==
                        mappings.get_apt_out(control_num).source_slots;
                    color = using_original_mapping ? MAPPING.color_1_id
                                                   : MAPPING.color_2_id;
                } else {
                    /*Check each slot if there is a mapping*/
                    for (uint8_t slot = 0; slot < NUMBER_OF_BOARD_SLOTS;
                         ++slot) {
                        // #if (!FADE_LED_LIMIT)
                        //                         info.blink_fade =
                        //                         SAMPLE_GLOBAL_500MS_TICK;
                        // #endif
                        if (check_if_slot_source(MAPPING, (slot_nums)slot)) {
                            switch (MAPPING.mode) {
                            case LED_STAGE:
                            case LED_STAGE_INVERT:
                                color = use_stage_coloring(
                                    state.slot_states[slot],
                                    mappings.in_map(control_num), MAPPING,
                                    MAPPING.mode == LED_STAGE_INVERT,
                                    limit_shade[control_num]);
                                break;

                            case LED_POSITION_1:
                            case LED_POSITION_2:
                            case LED_POSITION_3:
                            case LED_POSITION_4:
                            case LED_POSITION_5:
                            case LED_POSITION_6:
                            case LED_POSITION_7:
                            case LED_POSITION_8:
                            case LED_POSITION_9:
                            case LED_POSITION_10:
                                // derive the position compare for the mode
                                {
                                    const uint8_t position =
                                        MAPPING.mode - LED_POSITION_1;
                                    color = use_saved_position_coloring(
                                        MAPPING,
                                        (state.slot_states[slot]
                                             .on_saved_position_bitset &
                                         (1 << position)) != 0);
                                }
                                break;
                            default:
                                break;
                            }
                        }
                    }
                }
                break;
            default:
                error_print("ERROR: HID control (%d,%d) not supported",
                            USAGE_PAGE, USAGE_ID);
                break;
            }
        }

        state.mapping_colors[control_num] = color;
    }

    // Encoder the colors and brightnesses
    for (uint8_t control_num = 0;
         control_num < device.hid_data.num_of_output_controls; ++control_num) {
        const Hid_Mapping_control_out& MAPPING = mappings.out_map(control_num);
        const Hid_Control_info& CONTROL =
            device.hid_data.hid_out_controls[control_num];

        color_data color = color_id_to_data(state.mapping_colors[control_num]);

        if (MAPPING.type == HID_USAGE_PAGE_LED) {
            if (MAPPING.mode == LED_STAGE || MAPPING.mode == LED_STAGE_INVERT) {
                apply_slot_brightness(color, SAMPLE_BOARD_DIM, MAPPING);
            } else {
                color *= static_cast<uint8_t>(SAMPLE_BOARD_DIM * 255 / 100);
            }
        }

        if (limit_shade[control_num]) {
            // On limit, do a fade.
            if constexpr (FADE_LED_LIMIT) {
                color *= SAMPLE_FADE_VAL;
            } else {
                color = SAMPLE_GLOBAL_500MS_TICK ? color : COLOR_OFF;
            }
        }
        encode_color(state.last_out_report, CONTROL, color);
    }

    /*Only write data out to the USB device if Tx message is different than last
     * time*/
    if (state.resend_cntr-- == 0 ||
        memcmp(&state.last_out_report, &LAST_SERIALIZED,
               sizeof(LAST_SERIALIZED)) != 0) {
        /*Write the out report to the device*/
        auto _ = lock_guard(xSPI_Semaphore);
        usb_write(mappings.address(), device.hid_data.hid_out_ep,
                  device.hid_data.num_of_output_bytes, state.last_out_report,
                  0);
        state.resend_cntr = state.RESEND_CYCLES;
    }
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
static void encode_color(uint8_t serialized[3 * MAX_NUM_CONTROLS],
                         const Hid_Control_info& control, color_data color) {
    switch (control.report_size) /*In bits*/
    {
    case 1:
        encode_color_monochrome_1bit(serialized, control, color);
        break;

    case 8:
        encode_color_monochrome_8bit(serialized, control, color);
        break;

    case 16:
        encode_color_RG_16bit(serialized, control, color);
        break;

    case 24:
        encode_color_RGB_24bit(serialized, control, color);
        break;

    default:
        break;
    }
}
static void encode_color_monochrome_1bit(uint8_t buffer[3 * MAX_NUM_CONTROLS],
                                         const Hid_Control_info& control,
                                         color_data color) {
    const uint8_t MASK =
        1 << (control.bit_position - 8 * control.byte_position);
    if (color.as_monochrome_bit()) {
        buffer[control.byte_position] |= MASK;
    } else {
        buffer[control.byte_position] &= ~MASK;
    }
}
static void encode_color_monochrome_8bit(uint8_t buffer[3 * MAX_NUM_CONTROLS],
                                         const Hid_Control_info& control,
                                         color_data color) {
    configASSERT(control.bit_position == 8 * control.byte_position);

    buffer[control.byte_position] = color.as_monochrome_max();
}

static void encode_color_RG_16bit(uint8_t buffer[3 * MAX_NUM_CONTROLS],
                                  const Hid_Control_info& control,
                                  color_data color) {
    configASSERT(control.bit_position == 8 * control.byte_position);

    buffer[control.byte_position]     = color.green;
    buffer[control.byte_position + 1] = color.red;
}

static void encode_color_RGB_24bit(uint8_t buffer[3 * MAX_NUM_CONTROLS],
                                   const Hid_Control_info& control,
                                   color_data color) {
    configASSERT(control.bit_position == 8 * control.byte_position);

    buffer[control.byte_position]     = color.blue;
    buffer[control.byte_position + 1] = color.green;
    buffer[control.byte_position + 2] = color.red;
}

static color_data color_id_to_data(Led_color_id color) {
    static constexpr float DIMMING_PERCENTAGE = 12.5;
    static constexpr uint8_t DIMMING_COEFFICIENT =
        static_cast<uint8_t>(DIMMING_PERCENTAGE / 100 * 0xFF);
    switch (color) {
    case LED_ID_WHT:
        return {0xFF, 0xFF, 0xFF};
    case LED_ID_RED:
        return {0xFF, 0x00, 0x00};
    case LED_ID_GRN:
        return {0x00, 0xFF, 0x00};
    case LED_ID_BLU:
        return {0x00, 0x00, 0xFF};
    case LED_ID_YEL:
        return {0xFF, 0xFF, 0x00};
    case LED_ID_AQU:
        return {0x00, 0xFF, 0xFF};
    case LED_ID_MGN:
        return {0xFF, 0x00, 0xFF};
    case LED_ID_WHT_DIM:
        return color_data{0xFF, 0xFF, 0xFF} * DIMMING_COEFFICIENT;
    case LED_ID_RED_DIM:
        return color_data{0xFF, 0x00, 0x00} * DIMMING_COEFFICIENT;
    case LED_ID_GRN_DIM:
        return color_data{0x00, 0xFF, 0x00} * DIMMING_COEFFICIENT;
    case LED_ID_BLU_DIM:
        return color_data{0x00, 0x00, 0xFF} * DIMMING_COEFFICIENT;
    case LED_ID_YEL_DIM:
        return color_data{0xFF, 0xFF, 0x00} * DIMMING_COEFFICIENT;
    case LED_ID_AQU_DIM:
        return color_data{0x00, 0xFF, 0xFF} * DIMMING_COEFFICIENT;
    case LED_ID_MGN_DIM:
        return color_data{0xFF, 0x00, 0xFF} * DIMMING_COEFFICIENT;
    case LED_ID_OFF:
    default:
        return COLOR_OFF;
    }
}

static bool check_if_slot_source(const Hid_Mapping_control_out& mapping,
                                 slot_nums slot) {
    /*First check if the card is enabled*/
    return slots[slot].card_type > CARD_IN_SLOT_NOT_PROGRAMMED &&
           Tst_bits(mapping.source_slots, 1 << (uint8_t)slot);
}

static void apply_slot_brightness(color_data& ref, uint8_t board_dim,
                                  const Hid_Mapping_control_out& out_mapping) {
    auto first_non_off_color =
        [](std::initializer_list<Led_color_id> list) -> Led_color_id {
        configASSERT(!std::empty(list));
        auto itr = list.begin();
        while (itr != list.end() && *itr == LED_ID_OFF) {
            ++itr;
        }
        return *itr;
    };

    const color_data LIT_COLOR = color_id_to_data(
        first_non_off_color({out_mapping.color_1_id, out_mapping.color_2_id,
                             out_mapping.color_3_id}));

    for (slot_nums itr = SLOT_1; itr <= SLOT_8; ++itr) {
        Slots& slot = slots[itr];
        if ((out_mapping.source_slots & (1 << static_cast<uint8_t>(itr))) !=
            0) {
            // If the slot LED brightness is being manually controlled, this is
            // used to signal information.
            // For that reason, the lit fallback color should be used.
            if (slot.slot_led_brightness != 0xFF && ref == COLOR_OFF) {
                ref = LIT_COLOR;
            }

            // Brightness is in the range [0, 100].
            // 0xFF is a sentinel value indicating that
            // the value is begin controlled by the board.
            uint8_t brightness_perecentage = slot.slot_led_brightness;
            if (brightness_perecentage == 0xFF) {
                brightness_perecentage = board_dim;
            }

            // This moves the brightness into the [0, 255] range used
            // for color multiplication.
            ref *= static_cast<uint8_t>(brightness_perecentage * 255 / 100);
            break;
        }
    }
}

static Led_color_id use_stage_coloring(
    const drivers::usb_device::out_report_state::slot_state& slot_state,
    const Hid_Mapping_control_in& in_mapping,
    const Hid_Mapping_control_out& out_mapping, bool invert,
    bool& limit_shade) {
    Led_color_id target_color = LED_ID_OFF;

    /* Control Disabled */
    if (in_mapping.control_disable) {
        /*If Moving*/
        if (slot_state.is_moving()) {
            target_color = out_mapping.color_3_id;
        } else {
            /* Solid color 1*/
            target_color = out_mapping.color_1_id;
        }
    }
    /*Slot Disabled/Locked*/
    else if (!slot_state.is_enabled()) {
        /* Solid color 1*/
        target_color = out_mapping.color_1_id;
    }
    /*Slot Limit*/
    else if (slot_state.on_limit()) {
        target_color = invert ? out_mapping.color_2_id : out_mapping.color_1_id;
        limit_shade  = true;
    }
    /*Normal*/
    else {
        /*Moving*/
        if (slot_state.is_moving()) {
            /* Solid color 2*/
            target_color = out_mapping.color_3_id;
        }
        /*Idle*/
        else {
            /* Solid color 3*/
            target_color = out_mapping.color_2_id;
        }
    }

    return target_color;
}

static Led_color_id use_saved_position_coloring(
    const Hid_Mapping_control_out& out_mapping, bool on_position) {
    return on_position ? out_mapping.color_1_id : out_mapping.color_2_id;
}
