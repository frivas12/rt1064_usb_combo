/**
 * \file apt-command.cc
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 05-14-2024
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "apt-command.hh"

#include <algorithm>

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "integer-serialization.hh"
#include "portmacro.h"

using namespace drivers::usb;

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Macros
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/

/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

/*****************************************************************************
 * Static Data
 *****************************************************************************/

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
void apt_command_factory::receive_from_queue(apt_basic_command& dest,
                                             QueueHandle_t queue) {
    xQueueReceive(queue, &dest._message, portMAX_DELAY);
}
bool apt_command_factory::receive_from_queue(apt_basic_command& dest,
                                             QueueHandle_t queue,
                                             TickType_t timeout) {
    return xQueueReceive(queue, &dest._message, timeout) == pdTRUE;
}

apt_command apt_command_factory::receive_from_queue(QueueHandle_t queue) {
    apt_command rt;
    receive_from_queue(rt, queue);
    return rt;
}

std::optional<apt_command> apt_command_factory::receive_from_queue(
    QueueHandle_t queue, TickType_t timeout) {
    std::optional<apt_command> rt = std::optional<apt_command>(apt_command());

    if (!receive_from_queue(*rt, queue, timeout)) {
        rt.reset();
    }

    return rt;
}

void apt_command_factory::prepare_transaction_response(
    const apt_basic_command& inbound, apt_response& outbound) {
    auto serializer      = little_endian_serializer();
    auto response_buffer = std::span(outbound._buffer);
    // Header serialization
    serializer.serialize(response_buffer.template subspan<0, 2>(),
                         outbound.command);
    std::visit(
        [&response_buffer, &serializer](const auto& value) {
            using type = std::decay_t<decltype(value)>;

            if constexpr (std::same_as<type, apt_response::extended_data>) {
                const uint16_t SIZE = value.data.size();
                serializer.serialize(response_buffer.template subspan<2, 2>(),
                                     SIZE);
            } else {
                response_buffer[2] = std::byte(value.param1);
                response_buffer[3] = std::byte(value.param2);
            }
        },
        outbound.payload);
    response_buffer[4] = std::byte(
        inbound.source() | (outbound.payload.index() == 1 ? 0x80 : 0x00));
    response_buffer[5] = std::byte(inbound.destination());
}

void apt_command_factory::commit(const apt_basic_command& inbound,
                                 apt_response& outbound) {
    auto response_buffer = std::span(outbound._buffer);
    const std::size_t RESPONSE_SIZE =
        6 +
        std::visit(
            [](const auto& value) -> std::size_t {
                using type = std::decay_t<decltype(value)>;

                if constexpr (std::same_as<type, apt_response::extended_data>) {
                    return value.data.size();
                } else {
                    return 0;
                }
            },
            outbound.payload);

    // Halts if the response size is invalid.
    configASSERT(RESPONSE_SIZE <= apt_response::RESPONSE_BUFFER_SIZE);

    prepare_transaction_response(inbound, outbound);
    xSemaphoreTake(inbound._message->xUSB_Slave_TxSemaphore, portMAX_DELAY);
    inbound._message->write(response_buffer.data(), RESPONSE_SIZE);
    xSemaphoreGive(inbound._message->xUSB_Slave_TxSemaphore);
}

void apt_command_io_port::receive(apt_basic_command& dest) {
    apt_command_factory::receive_from_queue(dest, _io_queue);
}
apt_command apt_command_io_port::receive() {
    return apt_command_factory::receive_from_queue(_io_queue);
}

bool apt_command_io_port::receive(apt_basic_command& dest, TickType_t timeout) {
    return apt_command_factory::receive_from_queue(dest, _io_queue, timeout);
}
std::optional<apt_command> apt_command_io_port::receive(TickType_t timeout) {
    return apt_command_factory::receive_from_queue(_io_queue, timeout);
}

void apt_command_io_port::commit(const apt_basic_command& transaction,
                                 apt_response& response) {
    apt_command_factory::commit(transaction, response);
}

apt_command_io_port::apt_command_io_port(QueueHandle_t queue)
    : _io_queue(queue) {}

void apt_response::apply_builder(const apt_response_builder& builder) {
    command = builder.command();
    payload = builder.payload();
    if (extended_data* const data = std::get_if<extended_data>(&payload);
        data) {
        // If the span is not this response buffer, copy the data.
        std::span<std::byte, RESPONSE_BUFFER_SIZE> local = response_buffer();
        if (data->data.data() != local.data()) {
            std::ranges::copy(data->data, local.begin());
        }
    }
}

apt_response_builder apt_response::create_builder() {
    return apt_response_builder(response_buffer());
}

apt_command::apt_command() : apt_basic_command(_message) {}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

// EOF
