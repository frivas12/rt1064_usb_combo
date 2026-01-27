#pragma once

#include <cstdint>
namespace service::itc::message {

struct hid_out_message {
    uint16_t on_stored_position_bitset;

    uint8_t slot;
    uint8_t channel;

    bool on_limit;
    bool is_enabled;
    bool is_moving;

    constexpr static hid_out_message get_default(uint8_t slot,
                                                 uint8_t channel = 0) {
        return hid_out_message{
            .on_stored_position_bitset = 0,
            .slot                      = slot,
            .channel                   = channel,
            .on_limit                  = false,
            .is_enabled                = false,
            .is_moving                 = false,
        };
    }
};

}  // namespace serivce::itc::message
