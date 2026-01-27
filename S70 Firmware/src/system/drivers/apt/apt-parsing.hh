/**
 * \file apt-parsing.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 05-14-2024
 *
 * This file contains utilities for parsing APT commands.
 *
 * This file contains two utilities:
 * 1. Payload/Request structure serialization/deserializtion
 * 2. APT struct utilities
 *
 * This file defines two APT serialization and deserialization techniques.
 * The first technique "parameterization" defines a structure that can
 * be serialized and/or deserialized with the two one-byte parameters in a
 * short APT command.
 * The second technique "extended data" defines a structure that can
 * be serialized and/or deserialized with the extended data of the long
 * APT command.  This may be fixed length or variable.
 *
 * An APT "payload" structure defines one serialization and deserialization
 * type. An APT "request" structure defines one deserialization type. The
 * "parse_apt" function gives a common interface for deserialization of APT
 * "payload" and "request" types. The "encode_apt" function gives a common
 * interface for serializatin of APT "payload" types.
 *
 * An APT struct has one or more of the following criteria:
 * 1. Has a COMMAND_SET static member, a type "payload_type", and a factory
 * method payload_type::deserialize
 * 2. Has a COMMAND_REQ static member, a type "request_type", and a factory
 * method request_type::deserialize
 * 3. Has a COMMAND_GET static member, a type "payload_type", and a
 * serialization method payload_type::serialize()
 *
 * APT structs may also define the static member functions as customization
 * points:
 * - set(const apt_command&) -> parse_result<payload_type>
 * - req(const apt_command&) -> parse_result<request_type>
 * - get(apt_response_builder&, const payload_type&)
 *
 * The functions "apt_struct_set", "apt_struct_req", and "apt_struct_get"
 * provide a common interface for parsing and encoding the APT structures.
 * These functions will generate the common pattern to deserialize and APT
 * command and to serialize a paylode type. If the structure defines a
 * customization point member function, it will be used instead of the generated
 * pattern.
 */
#pragma once

#include <concepts>
#include <cstddef>
#include <expected>
#include <type_traits>

#include "apt-command.hh"

namespace drivers::apt {

enum class parser_errors_e {
    OKAY = 0,

    INVALID_COMMAND      = 1,
    INVALID_DESTINATION  = 2,
    INVALID_SOURCE       = 3,
    INVALID_PAYLOAD_MODE = 4,
    INVALID_PAYLOAD_SIZE = 5,
};

template <typename T>
using parser_result = std::expected<T, parser_errors_e>;

/// @brief Template that checks if a command is a parameter command.
auto parser_validate_parameter_command(
    std::invocable<const drivers::usb::apt_basic_command&> auto&& if_valid,
    const drivers::usb::apt_basic_command& command) {
    using return_t = parser_result<std::invoke_result_t<
        decltype(if_valid), const drivers::usb::apt_basic_command&>>;

    if (!command.has_parameters()) {
        return return_t(std::unexpect, parser_errors_e::INVALID_PAYLOAD_MODE);
    }

    return return_t(if_valid(command));
}

/// @brief Template that checks if a command has extended data of a certain
/// length.
auto parser_validate_extended_data_command(
    std::invocable<const drivers::usb::apt_basic_command&> auto&& if_valid,
    const drivers::usb::apt_basic_command& command,
    std::size_t extended_data_size) {
    using return_t = parser_result<std::invoke_result_t<
        decltype(if_valid), const drivers::usb::apt_basic_command&>>;

    if (!command.has_extended_data()) {
        return return_t(std::unexpect, parser_errors_e::INVALID_PAYLOAD_MODE);
    }

    if (command.extended_data().size() != extended_data_size) {
        return return_t(std::unexpect, parser_errors_e::INVALID_PAYLOAD_SIZE);
    }

    return return_t(if_valid(command));
}

template <typename T>
concept variable_span_deserializable = requires() {
    {
        T::deserialize(std::declval<std::span<const std::byte>>())
    } -> std::convertible_to<std::optional<T>>;
};

template <typename T>
concept fixed_span_deserializable = requires() {
    { T::APT_SIZE } -> std::convertible_to<std::size_t>;
    {
        T::deserialize(std::declval<std::span<const std::byte, T::APT_SIZE>>())
    } -> std::same_as<T>;
};

template <typename T>
concept parameter_deserializable = requires(uint8_t byte) {
    { T::deserialize(byte, byte) } -> std::same_as<T>;
};

template <typename T>
concept variable_span_serializable = requires(const T& t) {
    {
        t.serialize(
            std::declval<std::span<std::byte,
                                   usb::apt_response::RESPONSE_BUFFER_SIZE>>())
    } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept fixed_span_serializable = requires(const T& t) {
    t.serialize(std::declval<std::span<std::byte, T::APT_SIZE>>());
};

template <typename T>
concept parameter_serializable = requires(const T& t, uint8_t byte) {
    { t.serialize() } -> std::same_as<drivers::usb::apt_response::parameters>;
};

template <typename T>
concept apt_struct_with_set = requires(T t) {
    typename T::payload_type;
    {
        t.COMMAND_SET
    } -> std::same_as<const uint16_t&>;  // This is what a static field looks
                                         // like.
};

template <typename T>
concept apt_struct_with_req = requires(T t) {
    typename T::request_type;
    {
        t.COMMAND_REQ
    } -> std::same_as<const uint16_t&>;  // This is what a static field looks
                                         // like.
};
template <typename T>
concept apt_struct_with_get = requires(T t) {
    typename T::payload_type;
    {
        t.COMMAND_GET
    } -> std::same_as<const uint16_t&>;  // This is what a static field looks
                                         // like.
};

template <typename T>
concept apt_struct_specializes_set =
    requires(T t, const drivers::usb::apt_basic_command& command) {
        typename T::payload_type;
        {
            T::set(command)
        } -> std::same_as<parser_result<typename T::payload_type>>;
    };

template <typename T>
concept apt_struct_specializes_req =
    requires(T t, const drivers::usb::apt_basic_command& command) {
        typename T::request_type;
        {
            T::req(command)
        } -> std::same_as<parser_result<typename T::request_type>>;
    };
template <typename T>
concept apt_struct_specializes_get = requires(T t) {
    typename T::payload_type;
    T::get(std::declval<usb::apt_response_builder&>(),
           std::declval<const typename T::payload_type&>());
};

/// @brief Parses an extended-data APT structure with variable-length data.
template <variable_span_deserializable T>
parser_result<T> parse_apt(const drivers::usb::apt_basic_command& command) {
    using return_t = parser_result<T>;

    if (!command.has_extended_data()) {
        return return_t(std::unexpect, parser_errors_e::INVALID_PAYLOAD_MODE);
    }

    auto maybe = T::deserialize(command.extended_data());
    return maybe
               ? return_t(std::move(*maybe))
               : return_t(std::unexpect, parser_errors_e::INVALID_PAYLOAD_SIZE);
}

/// @brief Parses an extended-data APT structure.
template <fixed_span_deserializable T>
parser_result<T> parse_apt(const drivers::usb::apt_basic_command& command) {
    return parser_validate_extended_data_command(
        [](const drivers::usb::apt_basic_command& command) {
            return T::deserialize(
                command.extended_data().template subspan<0, T::APT_SIZE>());
        },
        command, T::APT_SIZE);
}

/// @brief Parses a parameterized APT structure.
template <parameter_deserializable T>
parser_result<T> parse_apt(const drivers::usb::apt_basic_command& command) {
    return parser_validate_parameter_command(
        [](const drivers::usb::apt_basic_command& command) {
            return T::deserialize(command.param1(), command.param2());
        },
        command);
}

/// @brief Encodes an APT response of the object with the associated command.
template <variable_span_serializable T>
usb::apt_response_builder& encode_apt(usb::apt_response_builder& builder,
                                      const T& obj, uint16_t command) {
    return builder.command(command).extended_data(
        [&obj](std::span<std::byte,
                         drivers::usb::apt_response::RESPONSE_BUFFER_SIZE>
                   buffer) {
            const uint32_t BYTES_TAKEN = obj.serialize(buffer);
            return buffer.subspan(0, BYTES_TAKEN);
        });
}

/// @brief Encodes an APT response of the object with the associated command.
template <fixed_span_serializable T>
usb::apt_response_builder& encode_apt(usb::apt_response_builder& builder,
                                      const T& obj, uint16_t command) {
    return builder.command(command).extended_data(
        [&obj](std::span<std::byte,
                         drivers::usb::apt_response::RESPONSE_BUFFER_SIZE>
                   buffer) {
            static_assert(T::APT_SIZE <=
                          drivers::usb::apt_response::RESPONSE_BUFFER_SIZE);
            obj.serialize(buffer.template subspan<0, T::APT_SIZE>());
            return buffer.template subspan<0, T::APT_SIZE>();
        });
}

/// \brief Encodes an APT response of the object with the associated command.
template <parameter_serializable T>
usb::apt_response_builder& encode_apt(usb::apt_response_builder& builder,
                                      const T& obj, uint16_t command) {
    return builder.command(command).parameters(obj.serialize());
}

/// \brief Template for parsing APT SET commands.
template <typename T>
    requires(apt_struct_with_set<T> &&
             (parameter_deserializable<typename T::payload_type> ||
              fixed_span_deserializable<typename T::payload_type> ||
              variable_span_deserializable<typename T::payload_type>)) ||
            apt_struct_specializes_set<T>
parser_result<typename T::payload_type> apt_struct_set(
    const drivers::usb::apt_basic_command& command) {
    if constexpr (apt_struct_specializes_set<T>) {
        return T::set(command);
    } else {
        return parse_apt<typename T::payload_type>(command);
    }
}

/// \brief Template for parsing APT REQ commands.
template <typename T>
    requires(apt_struct_with_req<T> &&
             (parameter_deserializable<typename T::request_type> ||
              fixed_span_deserializable<typename T::request_type>)) ||
            apt_struct_specializes_req<T>
parser_result<typename T::request_type> apt_struct_req(
    const drivers::usb::apt_basic_command& command) {
    if constexpr (apt_struct_specializes_req<T>) {
        return T::req(command);
    } else {
        return parse_apt<typename T::request_type>(command);
    }
}

/// \brief Template for encoding APT GET commands.
template <typename T>
    requires(apt_struct_with_get<T> &&
             (parameter_serializable<typename T::payload_type> ||
              fixed_span_serializable<typename T::payload_type> ||
              variable_span_serializable<typename T::payload_type>)) ||
            apt_struct_specializes_get<T>
void apt_struct_get(usb::apt_response_builder& builder,
                    const typename T::payload_type& payload) {
    if constexpr (apt_struct_specializes_get<T>) {
        T::get(builder, payload);
    } else {
        encode_apt(builder, payload, T::COMMAND_GET);
    }
}

/// \brief Template for encoding APT GET commands directly
///        into a apt_response object.
template <typename T>
    requires(apt_struct_with_get<T> &&
             (parameter_serializable<typename T::payload_type> ||
              fixed_span_serializable<typename T::payload_type> ||
              variable_span_serializable<typename T::payload_type>)) ||
            apt_struct_specializes_get<T>
void apt_struct_get(usb::apt_response& response,
                    const typename T::payload_type& payload) {
    auto builder = response.create_builder();
    apt_struct_get<T>(builder, payload);
    response.apply_builder(builder);
}

}  // namespace drivers::apt

// EOF
