#include "./flipper_shutter_shutter_common.hh"

using namespace drivers::save_constructor;

/**
 * Bitpattern:
 *  7 6 5 4 3 2 1 0
 *  R-R B-B-B A-A-A
 *  R = Reseved
 *  B = Power-Up State
 *  A = Default Trigger Mode
 */

flipper_shutter_shutter_common
flipper_shutter_shutter_common::deserialize(std::span<const std::byte, SIZE> src) {
    static constexpr std::byte MASK{0x07};
    return flipper_shutter_shutter_common{
        .power_up_state =
            static_cast<apt::shutter_types::states_e>((src[0] >> 3) & MASK),
        .default_trigger_mode =
            static_cast<apt::shutter_types::trigger_modes_e>(
                ((src[0] >> 0) & MASK)),
    };
}

void flipper_shutter_shutter_common::serialize(std::span<std::byte, SIZE> dest) const {
    static constexpr std::byte MASK{0x07};
    dest[0] = ((static_cast<std::byte>(power_up_state) & MASK) << 3) |
              ((static_cast<std::byte>(default_trigger_mode) & MASK) << 0);
}

// EOF
