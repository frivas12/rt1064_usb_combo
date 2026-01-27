#pragma once

#include <mutex>
#include <ranges>

#include "Debugging.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "rw-lock.hh"

namespace sync {

/**
 * A queue map is an associative container that allows registration
 * of queues with a key value.
 * When a producer uses "push_back" or "push_back_if", the list
 * of registered clients is used to forward the Message to the queues.
 * "push_back_if" provides the ability to select which queues to send based
 * on the queue's associated key value.
 *
 * The queue map has been designed around a templated container of
 * <Key, QueueHandle_t>.  The required interface for the container has been
 * targeted to std::map.
 */
template <typename Message, typename Key,
          template <typename> typename Container>
    requires std::ranges::common_range<
                 Container<std::pair<Key, QueueHandle_t>>> &&
             std::default_initializable<
                 Container<std::pair<Key, QueueHandle_t>>> &&
             requires(Container<std::pair<Key, QueueHandle_t>> c,
                      const std::pair<Key, QueueHandle_t>& v) {
                 c.insert(v);
                 c.erase(v);
                 {
                     *c.begin()
                 } -> std::convertible_to<std::pair<Key, QueueHandle_t>>;
             }
class queue_map {
   public:
    queue_map(rw_lock& lock) : _lock(lock) {}

    rw_lock& get_lock() const { return _lock; }

    auto register_queue(const Key& key, QueueHandle_t queue) {
        auto lock = _lock.writer_lock();
        auto _    = std::lock_guard(lock);
        return _clients.insert(std::pair(key, queue));
    }

    void unregister_queue(const Key& key) {
        auto lock = _lock.writer_lock();
        auto _    = std::lock_guard(lock);
        for (auto itr = _clients.begin(); itr != _clients.end(); ++itr) {
            if (itr->first == key) {
                _clients.erase(*itr);
                break;
            }
        }
    }

    void unregister_queue(const QueueHandle_t queue) {
        auto lock = _lock.writer_lock();
        auto _    = std::lock_guard(lock);
    // Loops until all the items with matching clients keys are removed.
    restart:
        for (auto itr = _clients.begin(); itr != _clients.end(); ++itr) {
            if (itr->second == queue) {
                goto restart;
            }
        }
    }

    std::size_t push_back(const Message& message,
                          TickType_t timeoutPerQueue = 0) const {
        std::size_t rt = 0;
        auto lock      = _lock.reader_lock();
        auto _         = std::lock_guard(lock);
        for (const std::pair<Key, QueueHandle_t>& pair : _clients) {
            if (pdTRUE != xQueueSend(pair.second, &message, timeoutPerQueue)) {
                ++rt;
            }
        }
        return rt;
    }

    std::size_t push_back_if(const Message& message,
                             std::predicate<const Key&> auto&& condition,
                             TickType_t timeoutPerQueue = 0) const {
        std::size_t rt = 0;
        auto lock      = _lock.reader_lock();
        auto _         = std::lock_guard(lock);
        for (const std::pair<Key, QueueHandle_t>& pair : _clients) {
            if (condition(pair.first)) {
                rt += (pdTRUE == xQueueSend(pair.second, &message, timeoutPerQueue)) ? 1 : 0;
            }
        }
        return rt;
    }

   private:
    Container<std::pair<Key, QueueHandle_t>> _clients;
    rw_lock& _lock;
};

}  // namespace sync
