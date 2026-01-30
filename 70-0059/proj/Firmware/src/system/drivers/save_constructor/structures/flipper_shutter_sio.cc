#include "flipper_shutter_sio.hh"

using namespace drivers::save_constructor;

/**
 * Bitpattern:
 *  7 6 5 4 3 2 1 0
 *  R-R-R-R-R-R-R A
 *  R = Reseved
 *  A = Power-Up Position (1 = 5V, 0 = 0V)
 */

flipper_shutter_sio
flipper_shutter_sio::deserialize(std::span<const std::byte, SIZE> src) {
    return flipper_shutter_sio{
        .start_up_position = (std::to_integer<int>(src[0]) & 0x01) != 0,
    };
}

void flipper_shutter_sio::serialize(std::span<std::byte, SIZE> dest) const {
    dest[0] = std::byte( start_up_position ? 1 : 0 );
}

// EOF
