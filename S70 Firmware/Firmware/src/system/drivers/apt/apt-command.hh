/**
 * \file apt-command.hh
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 05-14-2024
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include <concepts>
#include <cstddef>
#include <expected>
#include <optional>
#include <ranges>
#include <span>
#include <type_traits>
#include <variant>

#include "FreeRTOS.h"
#include "ptr-to-span.hh"
#include "queue.h"
#include "usb_slave.h"

namespace drivers::usb {

class apt_command;
class apt_basic_command;
class apt_response;

/// @brief Service to generate APT transactions.
class apt_command_factory {
   public:
    static void receive_from_queue(apt_basic_command& dest,
                                   QueueHandle_t queue);
    static bool receive_from_queue(apt_basic_command& dest, QueueHandle_t queue,
                                   TickType_t timeout);

    static apt_command receive_from_queue(QueueHandle_t queue);
    static std::optional<apt_command> receive_from_queue(QueueHandle_t queue,
                                                         TickType_t timeout);

    static void prepare_transaction_response(
        const apt_basic_command& transaction, apt_response& response);
    static void commit(const apt_basic_command& transaction,
                       apt_response& response);
};

/// @brief Simple wrapper class for the apt_command factory using an RX queue.
class apt_command_io_port {
   public:
    void receive(apt_basic_command& dest);
    apt_command receive();

    bool receive(apt_basic_command& dest, TickType_t timeout);
    std::optional<apt_command> receive(TickType_t timeout);

    void commit(const apt_basic_command& transaction, apt_response& response);

    explicit apt_command_io_port(QueueHandle_t queue);

    apt_command_io_port(apt_command_io_port&&) = delete;

   private:
    QueueHandle_t _io_queue;
};

/// \brief Concept of a function that fills a span.
/// \returns A subspan as a view over the data that was filled in.
template <typename T, typename ValueType, std::size_t Width>
concept span_filler =
    std::invocable<T, const std::span<ValueType, Width>&> &&
    std::convertible_to<
        std::invoke_result_t<T, const std::span<ValueType, Width>>,
        std::span<ValueType>>;

class apt_response_builder;

/**
 * A container for data used during the serialization of data into a new APT
 * command.
 */
class apt_response {
   public:
    /// \brief The required size of the respone buffer.
    static constexpr std::size_t RESPONSE_BUFFER_SIZE =
        (USB_SLAVE_BUFFER_SIZE - 6);

   private:
    std::array<std::byte, RESPONSE_BUFFER_SIZE + 6> _buffer;

   public:
    /// \brief Data structure for header-only commands.
    struct parameters {
        uint8_t param1;
        uint8_t param2;
    };

    /// \brief Data structure for extended-data commands.
    struct extended_data {
        std::span<std::byte> data;
    };

    uint16_t command;

    std::variant<parameters, extended_data> payload;

    inline std::span<std::byte, RESPONSE_BUFFER_SIZE> response_buffer() {
        return std::span(_buffer).template subspan<6, RESPONSE_BUFFER_SIZE>();
    }

    /// \brief Applies the builder to this response buffer.
    /// \note  This applies to both builders from create_builder()
    ///        as well as foreign builders.
    void apply_builder(const apt_response_builder& builder);

    /// \brief Creates a builder targetting the internal resources of the
    /// response.
    apt_response_builder create_builder();

    friend class apt_command_factory;
};

/**
 * A non-owning utility class that enables apt_response objects
 * to be filled.
 */
class apt_response_builder {
   public:
    constexpr apt_response_builder& command(uint16_t value) {
        _command = value;
        return *this;
    }

    constexpr apt_response_builder& parameters(apt_response::parameters value) {
        _payload = value;
        return *this;
    }

    template <span_filler<std::byte, apt_response::RESPONSE_BUFFER_SIZE> Filler>
    constexpr apt_response_builder& extended_data(Filler&& filler_function) {
        _payload =
            apt_response::extended_data{.data = filler_function(_buffer)};
        return *this;
    }

    constexpr uint16_t command() const { return _command; }

    inline const std::variant<apt_response::parameters,
                              apt_response::extended_data>&
    payload() const {
        return _payload;
    }

    constexpr apt_response_builder(
        const std::span<std::byte, apt_response::RESPONSE_BUFFER_SIZE>& buffer)
        : _payload(), _buffer(buffer) {}

   private:
    std::variant<apt_response::parameters, apt_response::extended_data>
        _payload;
    std::span<std::byte, apt_response::RESPONSE_BUFFER_SIZE> _buffer;
    uint16_t _command;
};

/// \brief A basic adapter on the USB_Slave_Message pointer to define an APT
/// command.
class apt_basic_command {
   public:
    inline uint16_t command() const { return _message->ucMessageID; }

    inline uint8_t destination() const { return _message->destination; }

    inline uint8_t source() const { return _message->source; }

    inline bool has_extended_data() const { return _message->bHasExtendedData; }
    inline bool has_parameters() const { return !has_extended_data(); }

    /// \pre this->has_parameters() == true
    inline uint8_t param1() const { return _message->param1; }

    /// \pre this->has_parameters() == true
    inline uint8_t param2() const { return _message->param2; }

    /// \pre this->() == true
    inline std::span<std::byte> extended_data() {
        uint8_t* ptr = _message->extended_data_buf;
        return std::as_writable_bytes(ptr2span(ptr, _message->ExtendedData_len));
    }

    /// \pre this->has_parameters() == true
    inline std::span<const std::byte> extended_data() const {
        const uint8_t* ptr = _message->extended_data_buf;
        return std::as_bytes(ptr2span(ptr, _message->ExtendedData_len));
    }

    apt_basic_command(USB_Slave_Message& message) : _message(&message) {}

    friend class apt_command_factory;

   private:
    USB_Slave_Message* _message;
};

/// \brief Extension of the apt_basic_command that owns its own slave message.
class apt_command : public apt_basic_command {
   public:
    friend class apt_command_factory;

   protected:
    apt_command();

   private:
    USB_Slave_Message _message;
};

template <std::invocable<apt_response_builder&> Visitor>
apt_response& build_response(apt_response& response, Visitor&& visitor) {
    auto builder = response.create_builder();
    visitor(builder);
    response.apply_builder(builder);
    return response;
}

}  // namespace drivers::usb

// EOF
