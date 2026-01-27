#pragma once

#include "joystick_map_in.hh"

namespace drivers::usbhid
{
    void apt_set_handler(const apt::joystick_map_in::payload_type& set);
    apt::joystick_map_in::payload_type apt_get_handler(const apt::joystick_map_in::request_type& request);
}

// EOF
