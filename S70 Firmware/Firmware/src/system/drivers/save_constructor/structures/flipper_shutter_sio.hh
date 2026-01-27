/**
* \file flipper_shutter_sio.hh
* \author Sean Benish (sbenish@thorlabs.com)
* \date 2024-06-21
*/
#pragma once

#include <span>

#include "../structure_id.hh"

namespace drivers::save_constructor {
    struct flipper_shutter_sio
    {
        static constexpr Struct_ID ID = Struct_ID::FLIPPER_SHUTTER_SIO;
        static constexpr std::size_t SIZE = 1;

        bool start_up_position;

        static flipper_shutter_sio deserialize(std::span<const std::byte, SIZE> src);
        void serialize(std::span<std::byte, SIZE> dest) const;
    };
}

// EOF

