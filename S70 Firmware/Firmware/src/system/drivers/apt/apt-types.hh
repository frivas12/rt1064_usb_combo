/**
 * \file apt-types.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 06-06-2024
 * 
 * @copyright Copyright (c) 2024
 * 
 * Common types shared among all APT commands.
 */
#pragma once

#include <cstdint>

namespace drivers::apt
{
    using channel_t = uint16_t;
}

// EOF