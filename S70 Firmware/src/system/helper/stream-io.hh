/**
 * \file stream-io.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 2024-04-19
 *
 */
#pragma once
#include <concepts>
#include <span>
#include <optional>
#include <utility>

namespace pnp_database {

namespace seeking {

struct begin_t {};
struct current_t {};
struct end_t {};

/// \brief Indicates that a seek should occur relative to the start of the
/// stream.
constexpr begin_t begin;

/// \brief INdicates that a seek should occur relative to the current position.
constexpr current_t current;

/// \brief Indicates that a sheek should occur relative to the end of the
/// stream.
constexpr end_t end;

/// \brief Checks if the type supports begin seeking.
template <typename T>
concept supports_begin_seeking = requires(T t, std::size_t addr) {
    { t.seek(seeking::begin, addr) } -> std::convertible_to<std::size_t>;
};

/// \brief Checks if the type supports relative to current location seeking.
template <typename T>
concept supports_current_seeking = requires(T t) {
    typename T::difference_type;
    {
        t.seek(seeking::current, std::declval<typename T::difference_type>())
    } -> std::convertible_to<std::size_t>;
};

/// \brief Checks if the type supports end seeking.
template <typename T>
concept supports_end_seeking = requires(T t, std::size_t addr) {
    { t.seek(seeking::end, addr) } -> std::convertible_to<std::size_t>;
};

} // namespace seeking

template <typename T>
concept input_stream = requires(T t, std::span<typename T::value_type> dest) {
    typename T::value_type;
    { t.read() } -> std::same_as<std::optional<typename T::value_type>>;
    { t.read(dest) } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept output_stream = requires(T t, typename T::value_type value,
                                 std::span<const typename T::value_type> src) {
    typename T::value_type;
    { t.write(value) } -> std::convertible_to<bool>;
    { t.write(src) } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept io_stream = input_stream<T> && output_stream<T>;

template <typename T>
concept addressable_stream = requires(T t) {
    /// \brief Obtains the location (address) in the stream.
    { t.address() } -> std::convertible_to<std::size_t>;
};

/// \brief Stream insertion operator for output streams.
template <output_stream T, std::convertible_to<typename T::value_type> U>
T &operator<<(T &stream, U &&value) {
    if constexpr (std::convertible_to<
                      U, std::span<const typename T::value_type>>) {
        stream.write(static_cast<std::span<typename T::value_type>>(value));
    } else {
        static_assert(std::convertible_to<U, typename T::value_type>, "invalid type");
        stream.write(static_cast<typename T::value_type>(value));
    }
}

/// \brief Stream extracction oerator for input strams.
template <input_stream T, typename U> T &operator>>(T &stream, U &&value) {
    if constexpr (std::convertible_to<U, std::span<typename T::value_type>>) {
        stream.read(static_cast<std::span<typename T::value_type>>(value));
    } else {
        static_assert(std::convertible_to<U, typename T::value_type &>, "invalid type");
        static_cast<typename T::value_type &>(value) = stream.read();
    }
}

} // namespace pnp_database

// EOF
