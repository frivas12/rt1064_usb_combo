/**
 * \file serialization.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \version 0.1
 * \date 05-14-2024
 *
 *
 */
#pragma once

#include <bit>
#include <cstdint>
#include <cstring>
#include <span>

template <typename T>
concept integer_serializer = requires(T t) {
    t.serialize(std::declval<std::span<std::byte, 8>>(),
                std::declval<int64_t>());
    t.serialize(std::declval<std::span<std::byte, 8>>(),
                std::declval<uint64_t>());
    t.serialize(std::declval<std::span<std::byte, 4>>(),
                std::declval<int32_t>());
    t.serialize(std::declval<std::span<std::byte, 4>>(),
                std::declval<uint32_t>());
    t.serialize(std::declval<std::span<std::byte, 2>>(),
                std::declval<int16_t>());
    t.serialize(std::declval<std::span<std::byte, 2>>(),
                std::declval<uint16_t>());
    t.serialize(std::declval<std::span<std::byte, 1>>(),
                std::declval<int8_t>());
    t.serialize(std::declval<std::span<std::byte, 1>>(),
                std::declval<uint8_t>());
};
template <typename T>
concept integer_deserializer = requires(T t) {
    {
        t.deserialize_int64(std::declval<std::span<const std::byte, 8>>())
    } -> std::same_as<int64_t>;
    {
        t.deserialize_uint64(std::declval<std::span<const std::byte, 8>>())
    } -> std::same_as<uint64_t>;
    {
        t.deserialize_int32(std::declval<std::span<const std::byte, 4>>())
    } -> std::same_as<int32_t>;
    {
        t.deserialize_uint32(std::declval<std::span<const std::byte, 4>>())
    } -> std::same_as<uint32_t>;
    {
        t.deserialize_int16(std::declval<std::span<const std::byte, 2>>())
    } -> std::same_as<int16_t>;
    {
        t.deserialize_uint16(std::declval<std::span<const std::byte, 2>>())
    } -> std::same_as<uint16_t>;
    {
        t.deserialize_int8(std::declval<std::span<const std::byte, 1>>())
    } -> std::same_as<int8_t>;
    {
        t.deserialize_uint8(std::declval<std::span<const std::byte, 1>>())
    } -> std::same_as<uint8_t>;
};

template <typename T>
concept float_serializer = requires(T t) {
    t.serialize(std::declval<std::span<std::byte, 8>>(),
                std::declval<double>());
    t.serialize(std::declval<std::span<std::byte, 4>>(), std::declval<float>());
};
template <typename T>
concept float_deserializer = requires(T t) {
    {
        t.deserialize_double(std::declval<std::span<const std::byte, 8>>())
    } -> std::same_as<double>;
    {
        t.deserialize_float(std::declval<std::span<const std::byte, 4>>())
    } -> std::same_as<float>;
};

/// @brief Utility class to
class native_endian_serializer {
   public:
    inline void serialize(std::span<std::byte, 8> buffer, int64_t value) {
        std::memcpy(buffer.data(), &value, buffer.size());
    }

    inline void serialize(std::span<std::byte, 8> buffer, uint64_t value) {
        std::memcpy(buffer.data(), &value, buffer.size());
    }

    inline void serialize(std::span<std::byte, 4> buffer, int32_t value) {
        std::memcpy(buffer.data(), &value, buffer.size());
    }

    inline void serialize(std::span<std::byte, 4> buffer, uint32_t value) {
        std::memcpy(buffer.data(), &value, buffer.size());
    }

    inline void serialize(std::span<std::byte, 2> buffer, int16_t value) {
        std::memcpy(buffer.data(), &value, buffer.size());
    }

    inline void serialize(std::span<std::byte, 2> buffer, uint16_t value) {
        std::memcpy(buffer.data(), &value, buffer.size());
    }

    inline void serialize(std::span<std::byte, 1> buffer, int8_t value) {
        std::memcpy(buffer.data(), &value, buffer.size());
    }

    inline void serialize(std::span<std::byte, 1> buffer, uint8_t value) {
        std::memcpy(buffer.data(), &value, buffer.size());
    }

    inline void serialize(std::span<std::byte, 8> buffer, double value) {
        std::memcpy(buffer.data(), &value, buffer.size());
    }

    inline void serialize(std::span<std::byte, 4> buffer, float value) {
        std::memcpy(buffer.data(), &value, buffer.size());
    }

    inline int64_t deserialize_int64(std::span<const std::byte, 8> buffer) {
        int64_t rt;
        std::memcpy(&rt, buffer.data(), buffer.size());
        return rt;
    }
    inline uint64_t deserialize_uint64(std::span<const std::byte, 8> buffer) {
        uint64_t rt;
        std::memcpy(&rt, buffer.data(), buffer.size());
        return rt;
    }

    inline int32_t deserialize_int32(std::span<const std::byte, 4> buffer) {
        int32_t rt;
        std::memcpy(&rt, buffer.data(), buffer.size());
        return rt;
    }
    inline uint32_t deserialize_uint32(std::span<const std::byte, 4> buffer) {
        uint32_t rt;
        std::memcpy(&rt, buffer.data(), buffer.size());
        return rt;
    }

    inline int16_t deserialize_int16(std::span<const std::byte, 2> buffer) {
        int16_t rt;
        std::memcpy(&rt, buffer.data(), buffer.size());
        return rt;
    }
    inline uint16_t deserialize_uint16(std::span<const std::byte, 2> buffer) {
        uint16_t rt;
        std::memcpy(&rt, buffer.data(), buffer.size());
        return rt;
    }

    inline int8_t deserialize_int8(std::span<const std::byte, 1> buffer) {
        int8_t rt;
        std::memcpy(&rt, buffer.data(), buffer.size());
        return rt;
    }
    inline uint8_t deserialize_uint8(std::span<const std::byte, 1> buffer) {
        uint8_t rt;
        std::memcpy(&rt, buffer.data(), buffer.size());
        return rt;
    }

    inline double deserialize_double(std::span<const std::byte, 8> buffer) {
        double rt;
        std::memcpy(&rt, buffer.data(), buffer.size());
        return rt;
    }

    inline float deserialize_float(std::span<const std::byte, 4> buffer) {
        float rt;
        std::memcpy(&rt, buffer.data(), buffer.size());
        return rt;
    }
};

using little_endian_serializer = native_endian_serializer;
static_assert(std::endian::native == std::endian::little,
              "little endian serialization class invalid");

/**
 * A builder-like adapter for simple serialization of integer values.
 * \warning All "write" operations are undefined if the stream does not have the
 * correct size.
 */
template <typename Serializer>
class stream_serializer {
   public:
    stream_serializer& write(int32_t value)
        requires integer_serializer<Serializer>
    {
        _ser.serialize(_working_buffer.subspan<0, 4>(), value);
        _working_buffer = _working_buffer.subspan(4);
        return *this;
    }

    stream_serializer& write(uint32_t value)
        requires integer_serializer<Serializer>
    {
        _ser.serialize(_working_buffer.subspan<0, 4>(), value);
        _working_buffer = _working_buffer.subspan(4);
        return *this;
    }

    stream_serializer& write(int16_t value)
        requires integer_serializer<Serializer>
    {
        _ser.serialize(_working_buffer.subspan<0, 2>(), value);
        _working_buffer = _working_buffer.subspan(2);
        return *this;
    }

    stream_serializer& write(uint16_t value)
        requires integer_serializer<Serializer>
    {
        _ser.serialize(_working_buffer.subspan<0, 2>(), value);
        _working_buffer = _working_buffer.subspan(2);
        return *this;
    }

    stream_serializer& write(int8_t value)
        requires integer_serializer<Serializer>
    {
        _ser.serialize(_working_buffer.subspan<0, 1>(), value);
        _working_buffer = _working_buffer.subspan(1);
        return *this;
    }

    stream_serializer& write(uint8_t value)
        requires integer_serializer<Serializer>
    {
        _ser.serialize(_working_buffer.subspan<0, 1>(), value);
        _working_buffer = _working_buffer.subspan(1);
        return *this;
    }

    stream_serializer& write(double value) requires float_serializer<Serializer> {
        _ser.serialize(_working_buffer.subspan<0, 8>(), value);
        _working_buffer = _working_buffer.subspan(8);
        return *this;
    }

    stream_serializer& write(float value) requires float_serializer<Serializer> {
        _ser.serialize(_working_buffer.subspan<0, 4>(), value);
        _working_buffer = _working_buffer.subspan(4);
        return *this;
    }

    std::size_t bytes_remaining() const { return _working_buffer.size(); }

    stream_serializer(std::span<std::byte> buffer, Serializer&& serializer)
        // : _full_buffer(buffer)
        : _working_buffer(buffer), _ser(std::forward<Serializer>(serializer)) {}

   private:
    // std::span<std::byte> _full_buffer;
    std::span<std::byte> _working_buffer;
    Serializer _ser;
};

/**
 * A builder-like adapter for simple deserialization of integer values.
 * \warning All "read" operations are undefined if the stream does not have the
 * correct size.
 */
template <typename Deserializer>
class stream_deserializer {
   public:
    template <typename T>
    T read() {
        T rt;
        std::size_t step;

        if constexpr (std::same_as<uint64_t, T>) {
            static_assert(integer_deserializer<Deserializer>);
            rt   = _ser.deserialize_uint32(_working_buffer.subspan<0, 8>());
            step = 8;
        } else if constexpr (std::same_as<int64_t, T>) {
            static_assert(integer_deserializer<Deserializer>);
            rt   = _ser.deserialize_int32(_working_buffer.subspan<0, 8>());
            step = 8;
        } else if constexpr (std::same_as<uint32_t, T>) {
            static_assert(integer_deserializer<Deserializer>);
            rt   = _ser.deserialize_uint32(_working_buffer.subspan<0, 4>());
            step = 4;
        } else if constexpr (std::same_as<int32_t, T>) {
            static_assert(integer_deserializer<Deserializer>);
            rt   = _ser.deserialize_int32(_working_buffer.subspan<0, 4>());
            step = 4;
        } else if constexpr (std::same_as<uint16_t, T>) {
            static_assert(integer_deserializer<Deserializer>);
            rt   = _ser.deserialize_uint16(_working_buffer.subspan<0, 2>());
            step = 2;
        } else if constexpr (std::same_as<int16_t, T>) {
            static_assert(integer_deserializer<Deserializer>);
            rt   = _ser.deserialize_int16(_working_buffer.subspan<0, 2>());
            step = 2;
        } else if constexpr (std::same_as<uint8_t, T>) {
            static_assert(integer_deserializer<Deserializer>);
            rt   = _ser.deserialize_uint8(_working_buffer.subspan<0, 1>());
            step = 1;
        } else if constexpr (std::same_as<int8_t, T>) {
            static_assert(integer_deserializer<Deserializer>);
            rt   = _ser.deserialize_int8(_working_buffer.subspan<0, 1>());
            step = 1;
        } else if constexpr (std::same_as<double, T>) {
            static_assert(float_deserializer<Deserializer>);
            rt = _ser.deserialize_double(_working_buffer.subspan<0, 8>());
            step = 8;
        } else if constexpr (std::same_as<float, T>) {
            static_assert(float_deserializer<Deserializer>);
            rt = _ser.deserialize_float(_working_buffer.subspan<0, 4>());
            step = 4;

        } else {
            static_assert(false, "invalid type provided");
        }

        _working_buffer = _working_buffer.subspan(step);
        return rt;
    }

    std::size_t bytes_remaining() const { return _working_buffer.size(); }

    stream_deserializer(std::span<const std::byte> buffer,
                        Deserializer&& deserializer)
        // : _full_buffer(buffer)
        : _working_buffer(buffer)
        , _ser(std::forward<Deserializer>(deserializer)) {}

   private:
    // std::span<std::byte> _full_buffer;
    std::span<const std::byte> _working_buffer;
    Deserializer _ser;
};

// EOF
