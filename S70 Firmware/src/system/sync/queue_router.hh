#pragma once

#include "queue_map.hh"

namespace sync {
/**
 * A queue router acts as a mediator object for many
 * queue objects of the Message type.
 * Clients can add their client-key-associated QueueHandle_t to the router
 * to receive messages.
 * Clients can also remove specific QueueHandle_t or ClientKey from the object.
 * Servers (associated with a ServerKey) can send Messages
 * through the queue_router to dispatch them to all clients where
 * the ClientKey and ServerKey satisfy the ForwardingPredicate.
 *
 * In addition, the ForwardingPredicate can be swapped out at runtime to
 * change the routing logic.
 */
template <typename Message, typename ClientKey, typename ServerKey,
          typename ForwardingPredicate, template <typename> typename Container>
    requires std::predicate<ForwardingPredicate, const ServerKey&,
                            const ClientKey&>
class queue_router {
   public:
    /// \brief Creates a new queue_router with the initial matching logic.
    ///        And
    queue_router(const ForwardingPredicate& matcher, rw_lock& lock)
        : _map(lock), _matcher(matcher) {}

    bool add(const ClientKey& client_key, QueueHandle_t queue) {
        return _map.register_queue(client_key, queue);
    }

    void remove(const ClientKey& client_key) {
        _map.unregister_queue(client_key);
    }

    void remove(const QueueHandle_t queue) { _map.unregister_queue(queue); }

    std::size_t route(const ServerKey& server_key, const Message& message,
                      TickType_t timeoutPerQueue = 0) const {
        return _map.push_back_if(
            message,
            [&](const ClientKey& key) { return _matcher(server_key, key); },
            timeoutPerQueue);
    }

    const ForwardingPredicate& forwarding_logic() const { return _matcher; }
    const ForwardingPredicate& forwarding_logic(
        const ForwardingPredicate& new_logic) {
        auto lock = _map.get_lock().writer_lock();
        auto _    = std::lock_guard(lock);
        _matcher  = new_logic;
        return _matcher;
    }

    template <std::invocable<ForwardingPredicate&> Visitor>
    auto with_forwarding_logic(Visitor&& visitor) {
        auto lock = _map.get_lock().writer_lock();
        auto _    = std::lock_guard(lock);
        return visitor(_matcher);
    }

   private:
    queue_map<Message, ClientKey, Container> _map;
    ForwardingPredicate _matcher;
};
}  // namespace sync
