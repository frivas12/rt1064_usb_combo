#pragma once

#include <cstddef>
#include <concepts>

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "UsbCore.h"
#include "apt.h"
#include "cdc-message.hh"
#include "hid-in-message.hh"
#include "hid-out-message.hh"
#include "inline-unordered-vector.hh"
#include "portmacro.h"
#include "projdefs.h"
#include "queue_map.hh"
#include "queue.h"
#include "rw-lock.hh"
#include "slot_nums.h"
#include "slots.h"
#include "task.h"

/**
 * The inter-thread communication service (ITC) defines
 * routes that allow application code to route messages between
 * threads.
 */
namespace service::itc {

/**
 * RAII handle to a unique queue with a type-erased
 * disposal method.
 */
template <typename T>
class queue_handle {
   public:
    using value_type = T;

    template <typename E>
        requires requires(E& e, QueueHandle_t q) { e.unregister_queue(q); }
    queue_handle(QueueHandle_t queue, E& container)
        : _dispose_parent([](void* parent, QueueHandle_t queue) {
            reinterpret_cast<E*>(parent)->unregister_queue(queue);
        })
        , _parent(&container)
        , _queue(queue) {}

    queue_handle(queue_handle<T>&& other)
        : _dispose_parent(other._dispose_parent)
        , _parent(other._parent)
        , _queue(other._queue) {
        other._queue = nullptr;
    }
    queue_handle& operator=(queue_handle<T>&& other) {
        try_dispose();
        _dispose_parent = other._dispose_parent;
        _queue          = other._queue;
        _parent         = other._parent;
        other._queue    = nullptr;
        return *this;
    }

    ~queue_handle() { try_dispose(); }

    QueueHandle_t handle() { return _queue; }

    void pop(T& out) {
        const bool GOOD = xQueueReceive(_queue, &out, portMAX_DELAY) == pdTRUE;
        configASSERT(GOOD);
    }

    bool try_pop(T& out, TickType_t timeout = 0) {
        return xQueueReceive(_queue, &out, timeout) == pdTRUE;
    }

    void pop_if(T& out, std::predicate<const T&> auto&& predicate) {
        do {
            pop(out);
        } while (!predicate(out));
    }

    bool try_pop_if(T& out, std::predicate<const T&> auto&& predicate,
                    TickType_t timeout = 0) {
        TickType_t now       = xTaskGetTickCount();
        const TickType_t END = now + timeout;
        while (now <= END) {
            if (try_pop(out, END - now) && predicate(out)) {
                return true;
            }
            now = xTaskGetTickCount();
        }

        return false;
    }

   private:
    void try_dispose() {
        if (_queue) {
            _dispose_parent(_parent, _queue);
        }
    }

    void (*_dispose_parent)(void*, QueueHandle_t);
    void* _parent;
    QueueHandle_t _queue;
};

template <typename Key, typename T, std::size_t Extent>
class pipeline {
   public:
    using value_type   = T;
    using unique_queue = queue_handle<value_type>;

    pipeline(std::size_t default_queue_size)
        : _lock(), _map(_lock), _default_queue_size(default_queue_size) {}

    // Pinned object
    pipeline(const pipeline<Key, T, Extent>&) = delete;
    pipeline(pipeline<Key, T, Extent>&&)      = delete;

    bool register_queue(const Key& key, QueueHandle_t queue) {
        return _map.register_queue(key, queue);
    }
    void unregister_queue(const Key& key) { _map.unregister_queue(key); }

    void unregister_queue(QueueHandle_t queue) { _map.unregister_queue(queue); }

    std::optional<unique_queue> create_queue(const Key& key) {
        return create_queue(key, _default_queue_size);
    }

    std::optional<unique_queue> create_queue(const Key& key,
                                             std::size_t queue_size) {
        QueueHandle_t handle = xQueueCreate(queue_size, sizeof(T));
        if (handle == nullptr) {
            return {};
        }

        register_queue(key, handle);
        return unique_queue(handle, *this);
    }

   protected:
    /**
     * Sends the value to the queue with the matching key.
     */
    auto send(const T& value, const Key& key) const {
        return _map.push_back_if(
            value, [&key](const Key& compare) { return key == compare; });
    }

    /**
     * Sends the value to any queue with a key satisfying the predicate.
     */
    auto send_if(const T& value,
                 std::predicate<const Key&> auto&& predicate) const {
        return _map.push_back_if(value, predicate);
    }

    /**
     * Sends the value to all queues.
     */
    auto broadcast(const T& value) const { return _map.push_back(value); }

   private:
    template <typename U>
    using map_container = inline_unordered_vector<U, Extent>;

    sync::rw_lock _lock;
    sync::queue_map<T, Key, map_container> _map;
    std::size_t _default_queue_size;
};

template <typename Key, typename T, std::size_t Extent>
class sender_filtered : public pipeline<Key, T, Extent> {
    using base_t = pipeline<Key, T, Extent>;

   public:
    // (sbenish) This is needed otherwise placement new's overload
    //           resolution does not deduce this constructor.
    sender_filtered(std::size_t default_queue_size)
        : base_t(default_queue_size) {}

    /**
     * Sends the value to the queue with the matching key.
     */
    auto send(const T& value, const Key& key) const {
        return pipeline<Key, T, Extent>::send(value, key);
    }

    /**
     * Sends the value to any queue with a key satisfying the predicate.
     */
    auto send_if(const T& value,
                 std::predicate<const Key&> auto&& predicate) const {
        return pipeline<Key, T, Extent>::send_if(
            value, std::forward<decltype(predicate)>(predicate));
    }
};

template <typename Key, typename T, std::size_t Extent>
class receiver_filtered : public pipeline<Key, T, Extent> {
    using base_t = pipeline<Key, T, Extent>;

   public:
    // (sbenish) This is needed otherwise placement new's overload
    //           resolution does not deduce this constructor.
    receiver_filtered(std::size_t default_queue_size)
        : base_t(default_queue_size) {}

    /**
     * Sends the value to all queues.
     */
    auto broadcast(const T& value) const {
        return pipeline<Key, T, Extent>::broadcast(value);
    }
};

using pipeline_hid_in_t =
    sender_filtered<slot_nums, service::itc::message::hid_in_message,
                    NUMBER_OF_BOARD_SLOTS>;
using pipeline_hid_out_t =
    receiver_filtered<uint8_t, service::itc::message::hid_out_message,
                      USB_NUMDEVICES>;
using pipeline_cdc_t =
    sender_filtered<asf_destination_ids, service::itc::message::cdc_message,
                    NUMBER_OF_BOARD_SLOTS>;

/**
 * Pipeline responsible for HID IN messages sent to slot cards from USB
 * device threads. Filtering is done on the sender's side.
 */
pipeline_hid_in_t& pipeline_hid_in();

/**
 * Pipeline responsible for HID OUT messages sent to USB devices from slot
 * cards.
 * Filtering is done on the receiver's side.
 */
pipeline_hid_out_t& pipeline_hid_out();

/**
 * Pipeline responsible for CDC messages (AKA APA commands) sent from
 * the FTDI or PC port to the slot cards.
 * Filtering is done on the sender's side.
 */
pipeline_cdc_t& pipeline_cdc();

void init();

// provide externs to the queue routers.

}  // namespace service::itc
