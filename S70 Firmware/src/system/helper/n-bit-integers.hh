/**
* \file n-bit-integers.hh
* \author Sean Benish (sbenish@thorlabs.com)
* \date 2024-06-21
* 
* A utility to enable working with n-bit integers.
* The main utility is being able to having the 
* compiler sign-extend integers.
*/

#pragma once

#include <cstddef>
#include <cstdint>

namespace utils::bit
{

template<std::size_t BitWidth>
struct intb
{
    int32_t val : BitWidth;
};
template<std::size_t BitWidth>
    requires (BitWidth > 32)
struct intb<BitWidth>
{
    int64_t val : BitWidth;
};
template<std::size_t BitWidth>
    requires (BitWidth <= 16 && BitWidth > 8)
struct intb<BitWidth>
{
    int16_t val : BitWidth;
};
template<std::size_t BitWidth>
    requires (BitWidth <= 8)
struct intb<BitWidth>
{
    int8_t val : BitWidth;
};

template<std::size_t BitWidth>
struct uintb
{
    uint32_t val : BitWidth;
};
template<std::size_t BitWidth>
    requires (BitWidth > 32)
struct uintb<BitWidth>
{
    uint64_t val : BitWidth;
};
template<std::size_t BitWidth>
    requires (BitWidth <= 16 && BitWidth > 8)
struct uintb<BitWidth>
{
    uint16_t val : BitWidth;
};
template<std::size_t BitWidth>
    requires (BitWidth <= 8)
struct uintb<BitWidth>
{
    uint8_t val : BitWidth;
};

}
