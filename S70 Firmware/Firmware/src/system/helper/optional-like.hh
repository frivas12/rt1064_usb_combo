/**
 * \file optional-like.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 2024-06-24
 *
 * Defines the optional-like concept
 */
#pragma once

#include <concepts>

/**
 * A concept that defines a type T is like an optional.
 * An optional must be convertible to bool to determine if it has an element.
 * If the conversion of bool is true, then the deference operator and arrow
 * operation must be defines the the const and non-const values.
 * The deference operator for an r-value reference shall also return a r-value
 * reference.
 */
template <typename T>
concept optional_like =
    std::default_initializable<T> && requires(T &t, const T &cr, T &&mv) {
        typename T::value_type;
        { static_cast<bool>(t) } -> std::same_as<bool>;
        { *t } -> std::same_as<typename T::value_type &>;
        { *cr } -> std::same_as<std::add_const_t<typename T::value_type &>>;
        { *mv } -> std::same_as<typename T::value_type &&>;
        { T::operator->(t) } -> std::same_as<typename T::value_type *>;
        {
            T::operator->(cr)
        } -> std::same_as<std::add_const_t<typename T::value_type *>>;
    };

// EOF
