/**
 * \file flipper-shutter-types.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 05-10-2024
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include "shutter-types.hh"

namespace cards::flipper_shutter {

using shutter_types_e = drivers::apt::shutter_types::types_e;
using trigger_modes_e = drivers::apt::shutter_types::trigger_modes_e;

enum class mirror_starting_positions_e {
    OPEN = 0,
    CLOSED = 1,
    IGNORE = 2,
};

} // namespace cards::flipper_shutter

// EOF
