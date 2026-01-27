#pragma once

#include <bitset>
#include "hid-mapping.hh"
#include "hid_out.h"
#include "hid_report.h"
#include "itc-service.hh"
#include "slots.h"
#include "usb_device.h"

namespace drivers::usb_device {

class out_report_state {
   public:
    struct slot_state {
        uint16_t on_saved_position_bitset;
        uint8_t _movement_bitset;
        std::bitset<1> _general_bitset;

        constexpr slot_state()
            : on_saved_position_bitset(0)
            , _movement_bitset(0)
            , _general_bitset(0)
        {}

        constexpr bool is_moving() const { return _movement_bitset & 0x01; }
        constexpr slot_state& is_moving(bool value) {
            if (value) {
                _movement_bitset |= 0x01;
            } else {
                _movement_bitset &= ~0x01;
            }

            return *this;
        }

        constexpr bool on_limit() const { return _movement_bitset & 0x80; }
        constexpr slot_state& on_limit(bool value) {
            if (value) {
                _movement_bitset |= 0x80;
            } else {
                _movement_bitset &= ~0x80;
            }

            return *this;
        }
        
        constexpr bool is_enabled() const { return _general_bitset.test(0); }
        constexpr void is_enabled(bool value) { _general_bitset.set(0, value); }
    };

    // Resends every N cycles (of N x 10 ms)
    static constexpr uint8_t RESEND_CYCLES = 25;

    // x3 for 24-bit colors.
    uint8_t last_out_report[3 * MAX_NUM_CONTROLS];
    Led_color_id mapping_colors[MAX_NUM_CONTROLS];
    slot_state slot_states[NUMBER_OF_BOARD_SLOTS];
    uint8_t resend_cntr;

    out_report_state(uint8_t address);
};

void service_in_report(UsbDevice& device,
                       service::hid_mapping::address_handle& mappings);

/**
 *  \param[inout] previous_info
 */
void service_out_report(
    UsbDevice& device,
    service::itc::queue_handle<service::itc::message::hid_out_message>& queue,
    service::hid_mapping::address_handle& mappings, out_report_state& state);
}  // namespace drivers::usb_device
