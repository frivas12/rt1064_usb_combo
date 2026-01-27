#pragma once

#include <concepts>
#include <optional>
#include <utility>

#include "FreeRTOS.h"
#include "task.h"

#include "gate.h"

namespace cards {

template <typename Card>
concept card_thread_local_object = requires(
    Card c, typename Card::initial_parameter_type &&initial_parameters,
    typename Card::attached_subobject_type &subobject, TickType_t now,
    TickType_t time_to_sleep) {
    /// \brief The initial parameter type for the card.
    typename Card::initial_parameter_type;

    /// \brief  A static function that consumes the initial parameters to
    ///         create the thread-local object for the card.
    ///         This will consume the initial parameters and return a
    ///         thread-local object.
    { Card::create(initial_parameters) } -> std::same_as<Card>;

    /// \brief Obtains the connection gate for the device connected to the
    /// card.
    { c.get_gate() } -> std::same_as<GateHandle_t>;

    /// \brief Gets the service period of the card.
    { c.get_service_period() } -> std::convertible_to<TickType_t>;

    /// \brief Suspends the current thread until the wakeup time
    ///        or some other card-defined wakeup criteria occurs.
    c.wait_until_awoken(time_to_sleep);

    /// \brief Factory function to create the attached subobject.
    /// \pre Gate is open.
    {
        c.try_attach()
    } -> std::convertible_to<bool>;

    /// \brief Services the subobject until the subobject detaches.
    c.service(now);

    /// \brief Cleans up resources after servicing.
    c.post_service_cleanup();
};

template <typename Card>
concept card_thread_local_object_with_idle_service =
    card_thread_local_object<Card> &&
    requires(Card c, TickType_t now) { c.service_idle(now); };

template <card_thread_local_object Card>
void card_thread(typename Card::initial_parameter_type *const owned_params) {
    Card card = Card::create(std::move(*owned_params));
    delete owned_params;

    TickType_t periodic_service_time;
    static auto service_wakeup = [&periodic_service_time,
                                  &card](TickType_t start_service,
                                         TickType_t period) {
        const TickType_t NOW = xTaskGetTickCount();

        // Keeping the periodic service time aligned to the period.
        while (periodic_service_time <= NOW) {
            periodic_service_time += period;
        }

        card.wait_until_awoken(periodic_service_time - NOW);
    };

    for (;;) {

        if constexpr (card_thread_local_object_with_idle_service<Card>) {
            const TickType_t PERIOD = card.get_service_period();
            periodic_service_time = xTaskGetTickCount();
            while (!xGatePass(card.get_gate(), 0)) {
                const TickType_t NOW = xTaskGetTickCount();
                card.service_idle(NOW);
                service_wakeup(NOW, PERIOD);
            }
        } else {
            xGatePass(card.get_gate(), portMAX_DELAY);
        }

        if (!card.try_attach()) {
            continue;
        }

        const TickType_t PERIOD = card.get_service_period();
        periodic_service_time = xTaskGetTickCount();
        while (xGatePass(card.get_gate(), 0)) {
            const TickType_t NOW = xTaskGetTickCount();
            card.service(NOW);
            service_wakeup(NOW, PERIOD);
        }

        card.post_service_cleanup();
    }
}

/// \brief Card thread template using a type-erased (void pointer) parameters
///        provided by the FreeRTOS API.
template <card_thread_local_object Card>
inline void type_erased_card_thread(void *owned_type_erased_params) {
    card_thread<Card>(
        reinterpret_cast<typename Card::initial_parameter_type *const>(
            owned_type_erased_params));
}

} // namespace cards

// EOF
