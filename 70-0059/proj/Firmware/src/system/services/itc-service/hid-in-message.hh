#pragma once

#include "hid_in.h"
namespace service::itc::message {

struct hid_in_message {
    Control_Modes mode;
    float control_data;

    uint16_t usage_page;
    uint16_t usage_id;

    uint8_t address;
    // uint8_t speed_modifier;
    // uint8_t deadband;
    uint8_t destination_channel_bitset;
    uint8_t destination_virtual_bitset;

    // bool is_reversed;
    
    static hid_in_message get_default(uint8_t address) {
        return {
            .mode = CTL_AXIS,
            .control_data = 0.0,
            .usage_page = 0,
            .usage_id = 0,
            .address = address,
            .destination_channel_bitset = 0,
            .destination_virtual_bitset = 0,
        };
    }
};

}  // namespace serivce::itc::message
