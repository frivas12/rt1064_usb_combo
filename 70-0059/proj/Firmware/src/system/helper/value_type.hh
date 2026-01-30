/**
 * \file value_type.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 2024-06-25
 */
#pragma once

#include <concepts>
namespace utils
{

template<typename T, typename Value>
concept is_value_type = std::same_as<typename T::value_type, Value>;

}

// EOF

