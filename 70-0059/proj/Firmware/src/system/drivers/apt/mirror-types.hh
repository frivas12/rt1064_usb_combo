/**
 * \file mirror-types.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 06-06-2024
 * 
 * @copyright Copyright (c) 2024
 * 
 * Types common to mirror commands
 */
#pragma once

namespace drivers::apt::mirror_types
{

enum class states_e
{
    OUT = 0,
    IN = 1,
    UNKNOWN = 2,
};

enum class modes_e
{
    DISABLED = 0,
    DIRECT = 1,
    SIGNAL = 2,
};

}

// EOF
