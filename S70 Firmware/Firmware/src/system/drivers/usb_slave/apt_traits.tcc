#pragma once

#include <type_traits>

namespace traits
{
/**
 * A type trait defining how large an object with a fixed size will be when serialized
 * into the APT data stream.
 * \note Template instantiation outside of this translation unit define explicit instantiations using the "constructor::apt_serialized_size" as its base class.
 */
template<typename T>
struct apt_serialized_size;

namespace constructor
{
template<std::size_t SIZE>
struct apt_serialized_size : public std::integral_constant<std::size_t, SIZE>
{ };
}

// TODO(sbenish):  The C++17 standard denotes that this variables may be inline, but the compiler doesn't like that.  arm is a pain :/
template<typename T>
constexpr std::size_t apt_serialized_size_v = apt_serialized_size<T>::value;

}



// EOF
