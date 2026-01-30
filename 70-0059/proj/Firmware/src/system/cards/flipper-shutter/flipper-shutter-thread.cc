/**
 * \file flipper-shutter-thread.cc
 * \author Sean Benish (sbenish@thorlabs.com)
 * \date 05-08-2024
 *
 * @copyright Copyright (c) 2024
 *
 * The flipper shutter card combines the TLL output behavior
 * of the Slider IO card w/ the PWM CPLD behavior of the 4-channel
 * shutter card.
 * The flipper shutter is hardware-compatible with existing
 * TTL Slider IO cables and 4-channel shutter cables.
 *
 * From a communication standpoint, the flipper shutter exposes
 * 7 channels:
 * 0 - Shutter 1
 * 1 - Shutter 2
 * 2 - Shutter 3
 * 3 - Shutter 4
 * 4 - TTL Output 1 (SIO0)
 * 5 - TTL Output 2 (SIO1)
 * 6 - TTL Output 3 (SIO3)
 * Shutter APT commands are accepted for channels [0, 3].
 * Mirror APT commands are accepted for channels [4, 6].
 * Note that the MGMSG_MCM_SET_MIRROR_PARAMS command is
 * ignored.
 *
 * Setup tasks for the flipper shutter:
 * 1.   Configure the hardware for common state.
 * 2.   Wait for a device to connect.
 * 3.   Set the interlock pin high to disable the shutters.
 *
 * Connection tasks for the flipper shutter:
 * 1.   Parse the device information.
 * 2.   Determine which channels are enabled with the attached cable.
 * 3.   Load the configuration into the driver.
 * 4.   Set the interlock pin to input to wait for an interlock signal.
 * 5.   Disable the shutter channels.
 * 6.   Enable the power.
 *
 * Running tasks that need to occur with the flipper shutter:
 * 1.   When the INT pin is raised due to a TTL change on the input GPIO pins,
 *      dispatch the associated action.
 * 2.   When an interrupt action is received to set, reset, or toggle the
 * shutter, respond to the action w/in 10 ms.
 * 3.   When a command is issued to set, reset, or toggle the flipper mirorrs,
 *      respond to the action w/in 10 ms.
 * 4.   Parse any APT commands.
 * 5.   Service the shutter every 10 milliseconds.
 *
 * Disconnect tasks:
 * 1.   Set the interlock pin high to disable the shuters.
 * 2.   Disable the power.
 */

#include "flipper-shutter-thread.hh"

#include <ranges>

#include "FreeRTOSConfig.h"
#include "apt-command.hh"
#include "card-thread.hh"
#include "channels.hh"
#include "cpld-shutter-driver.hh"
#include "defs.h"
#include "device_detect.h"
#include "flipper-shutter-driver.hh"
#include "flipper-shutter-persistence.hh"
#include "flipper-shutter-query.hh"
#include "flipper-shutter-thread.h"
#include "flipper-shutter-types.hh"
#include "fs-mirror-module.hh"
#include "fs-shutter-module.hh"
#include "hid.h"
#include "hid_out.h"
#include "itc-service.hh"
#include "lock_guard.hh"
#include "mcm_pwm_period.hh"
#include "mcm_shuttercomp_params.hh"
#include "mcp-driver.hh"
#include "mirror-types.hh"
#include "projdefs.h"
#include "shutter-controller.hh"
#include "shutter-types.hh"
#include "slot_nums.h"
#include "slot_types.h"
#include "slots.h"
#include "spi-transfer-handle.hh"
#include "sys_task.h"
#include "time.hh"
#include "usb_device.h"
#include "usb_slave.h"
#include "user-eeprom.hh"
#include "usr_interrupt.h"

// APT commands

#include "apt-parsing.hh"
#include "mcm_interlock_state.hh"
#include "mcm_mirror_params.hh"
#include "mcm_mirror_state.hh"
#include "mcm_shutter_params.hh"
#include "mod_chanenablestate.hh"
#include "mot_eepromparams.hh"
#include "mot_solenoid_state.hh"

using namespace cards::flipper_shutter;
using namespace service::itc;

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Macros
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/
enum class interlock_servicing_states_e {
    // When the interlock is ignored and the bridges are being driven.
    IGNORED_BRIDGES_SLEEPING,

    // Reading the interlock pin, waiting for the interlock to activate.
    WAITING_FOR_ACTIVATION,

    // Interlock has been activated (which may have consequences that need to be
    // handled).
    ACTIVATED_UNHANDLED_CONSEQUENCES,

    // The consequences are begin handled.  The interlock pin is being driven to
    // wake the bridges.
    HANDLING_CONSEQUENCES_WITH_AWOKEN_BRIDGES,

    // Interlock is activated and the consequences has been handled.  Interlock
    // pin is back in read-only mode.
    ACTIVATED_HANDLED_CONSEQUENCES,
};

class thread_local_object {
   public:
    /// @brief Parameters passed to the thread-local object on thread
    /// initialization.
    struct initial_parameter_type {
        TickType_t service_period;
        TickType_t watchdog_service_timeout;
        TickType_t watchdog_configuring_timeout;
        slot_types card_type;
        slot_nums slot;
    };

    /// @brief Object created after an object is attached.
    struct attached_subobject_type {
        static constexpr std::size_t SHUTTERS =
            std::size(cards::flipper_shutter::driver::CHANNEL_RANGE_SHUTTER);
        static constexpr std::size_t MIRRORS =
            std::size(cards::flipper_shutter::driver::CHANNEL_RANGE_SIO);
        static constexpr std::size_t CHANNELS = SHUTTERS + MIRRORS;

        device_signature_t connected_device;
        uint64_t connected_sn;
    };

    static thread_local_object create(initial_parameter_type parameters);

    GateHandle_t get_gate();

    TickType_t get_service_period() const;

    bool try_attach();

    void wait_until_awoken(TickType_t sleep_duration);

    void service_idle(TickType_t now);

    void service(TickType_t now);

    void post_service_cleanup();

    inline slot_nums get_slot() const { return _driver.get_slot(); }

    inline slot_types get_slot_type() const {
        return slots[get_slot()].card_type;
    }

   private:
    struct state_t {
        std::array<modules::shutter, attached_subobject_type::SHUTTERS>
            shutters;
        std::array<modules::mirror, attached_subobject_type::SHUTTERS> mirrors;
        bool interlock_cable_enabled                  = false;
        bool shutter_ttl_control_disables_usb_control = false;

        void open_shutter(driver::channel_id channel);
        void close_shutter(driver::channel_id channel);
        void toggle_shutter(driver::channel_id channel);
    };

    // (sbenish)  I moved a lot of the big optional objects
    //            into the card to better track them.
    state_t _state;
    std::optional<drivers::eeprom::page_cache> _cache;
    drivers::usb::apt_response _response;
    driver _driver;
    std::optional<attached_subobject_type> _subobject;
    interlock_servicing_states_e _interlock_state;
    ::service::itc::queue_handle<::service::itc::message::hid_in_message>
        _joystick_in_queue;
    ::service::itc::queue_handle<::service::itc::message::cdc_message>
        _cdc_queue;
    TickType_t _service_period;
    TickType_t _watchdog_service_timeout;
    TickType_t _watchdog_configuring_timeout;
    TickType_t _last_time_expensive_serviced;
    TickType_t _shutter_next_service_delay = portMAX_DELAY;
    // slot_types _card_type;

    // The power channel is not exposed to external code.
    std::bitset<std::size(driver::CHANNEL_RANGE_SHUTTER) +
                std::size(driver::CHANNEL_RANGE_SIO)>
        _channel_enabled;

    // \note The factory will not be owned, only used during construction.
    explicit thread_local_object(const initial_parameter_type& params,
                                 drivers::spi::handle_factory& factory);

    constexpr persistence::envir_context get_shutter_envir_context() const {
        return {
            .bus_voltage = 24,
            .pwm_period  = _driver.get_pwm_period(),
        };
    }

    void service_interlock(drivers::spi::handle_factory&);
    void service_joystick(drivers::spi::handle_factory&);
    bool parse_usb_cmd(drivers::spi::handle_factory&,
                       drivers::usb::apt_basic_command&);
    void service_apt(drivers::spi::handle_factory& factory);
    void service_trigger_action(cards::shutter::controller& controller,
                                trigger_modes_e mode, bool level);

    auto with_persistence_controller(auto&& visitor,
                                     drivers::spi::handle_factory& factory) {
        auto layout = persistence::layout_25lc1024();
        if (!_cache) {
            _cache.emplace(factory,
                           layout.card_settings(get_slot()).head_address);
        }
        drivers::eeprom::flush_guard fg(
            factory,
            *this->_cache);  // Guarantees a flush before the function exists.
        drivers::eeprom::page_cache& cache = *this->_cache;
        auto controller = persistence::persistence_controller(
            get_slot(), layout, factory, cache);
        static_assert(
            std::invocable<decltype(visitor), decltype(controller)&> ||
                std::invocable<decltype(visitor),
                               const decltype(controller)&> ||
                std::invocable<decltype(visitor), decltype(controller)>,
            "visitor not supported");
        return visitor(controller);
    }

    TickType_t get_time_until_next_shutter_service(TickType_t now) const;
};
static_assert(cards::card_thread_local_object<thread_local_object>);

/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
/// \pre slot <= SLOT_7
static void raise_smio_interrupt_from_isr(
    slot_nums slot, portBASE_TYPE& higher_priority_task_awoken);

static driver::creation_options create_driver_options(
    const thread_local_object::initial_parameter_type& params) noexcept;

/*****************************************************************************
 * Static Data
 *****************************************************************************/

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/
static void on_trigger_interrupt(uint8_t slot, void*);

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
TaskHandle_t cards::flipper_shutter::spawn_thread(slot_nums slot) {
    static constexpr std::array SUPPORTED_CARD_TYPES{
        MCM_Flipper_Shutter,
        MCM_Flipper_Shutter_REVA,
    };

    // Slot Checking
    if (slot > SLOT_7) {
        return NULL;
    }

    // Hardware Checking
    if (!std::ranges::contains(SUPPORTED_CARD_TYPES, slots[slot].card_type)) {
        return NULL;
    }

    // Slot owned checking
    if (xSlotHandle[static_cast<int>(slot)] != NULL) {
        return NULL;
    }

    char pcName[6] = "FlShX";
    pcName[4]      = '0' + static_cast<int>(slot);

    thread_local_object::initial_parameter_type* const passed_arguments =
        new thread_local_object::initial_parameter_type();

    if (passed_arguments == nullptr) {
        return NULL;
    }

    // Set the parameters.
    passed_arguments->slot           = slot;
    passed_arguments->card_type      = slots[slot].card_type;
    passed_arguments->service_period = FLIPPER_SHUTTER_UPDATE_INTERVAL;
    passed_arguments->watchdog_service_timeout =
        FLIPPER_SHUTTER_HEARTBEAT_INTERVAL;
    passed_arguments->watchdog_configuring_timeout =
        FLIPPER_SHUTTER_CONFIGURING_INTERVAL;

    TaskHandle_t rt = NULL;
    // static constexpr uint16_t STACK_SIZE = TASK_FLIPPER_SHUTTER_STACK_SIZE;
    static constexpr uint16_t STACK_SIZE = 800;
    if (xTaskCreate(&cards::type_erased_card_thread<thread_local_object>,
                    pcName, STACK_SIZE, passed_arguments,
                    TASK_FLIPPER_SHUTTER_PRIORITY, &rt) == pdPASS) {
        xSlotHandle[static_cast<int>(slot)] = rt;
    } else {
        delete passed_arguments;
    }

    return rt;
}

extern "C" void flipper_shutter_init(slot_nums slot, slot_types type) {
    xSlotHandle[slot] = cards::flipper_shutter::spawn_thread(slot);
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
thread_local_object thread_local_object::create(
    initial_parameter_type parameters) {
    // (sbenish) A delay is needed to allow the supervisor to finish
    //           handshaking with the CPLD.
    vTaskDelay(pdMS_TO_TICKS(20));
    // Put this before the factory b/c the factory will acquire the lock.

    drivers::spi::handle_factory factory;

    auto rt = thread_local_object(parameters, factory);
    // The driver object's create() method will setup the initial state.

    slots[parameters.slot].p_interrupt_smio_handler = &on_trigger_interrupt;
    setup_slot_interrupt_SMIO(parameters.slot);

    return rt;
}

GateHandle_t thread_local_object::get_gate() {
    return slots[get_slot()].device.connection_gate;
}

TickType_t thread_local_object::get_service_period() const {
    return _service_period;
}

bool thread_local_object::try_attach() {
    persistence::query_params params;
    device_signature_t connected_device;
    params.connected_card = get_slot_type();
    {
        lock_guard _(slots[get_slot()].xSlot_Mutex);

        params.connected_sn = slots[get_slot()].device.info.serial_num;
        connected_device    = slots[get_slot()].device.info.signature;
    }

    drivers::spi::handle_factory factory{};

    // If the state of the thread is not a setup query, make a setup query
    // and query the EEPROM.
    // This allows for future code with IDLE commands to access and modify
    // the setup query.

    persistence::setup_query setup;
    with_persistence_controller(
        [&setup, &params,
         ENABLE_DD = slots[get_slot()].save.allow_device_detection](
            persistence::persistence_controller<persistence::layout_25lc1024>&
                ctrler) {
            persistence::apply_query_to_setup(params, setup, ctrler,
                                              !ENABLE_DD);
        },
        factory);

    // (sbenish) If there were any reason to reject the cable, do it here.

    // (sbenish)  B/C rejected cables are called at the device-detection
    // level and look-up tables are not being used, there is no reason
    // to just try to run the currently connected cable.
    // If the cable is not valid, we just load the default values.

    // Load card-related settings.
    if (setup.get_card_settings() == nullptr) {
        setup.add(persistence::card_settings::get_defaults());
    }
    // Load the default mirror settings if they do not exist.
    for (driver::channel_id channel :
         utils::as_iterable(driver::CHANNEL_RANGE_SIO)) {
        if (setup.get_sio_settings(channel) == nullptr) {
            setup.add(persistence::sio_settings::get_defaults(), channel);
        }
    }

    _state.interlock_cable_enabled =
        setup.get_card_settings()->enable_interlock_cable;
    _state.shutter_ttl_control_disables_usb_control =
        setup.get_card_settings()->shutter_ttl_control_disables_usb_control;

    // Load mirror modules.
    for (driver::channel_id channel :
         utils::as_iterable(driver::CHANNEL_RANGE_SIO)) {
        auto& module = _state.mirrors[utils::channel_to_index(
            channel, driver::CHANNEL_RANGE_SIO)];
        if (auto* settings = setup.get_sio_settings(channel); settings) {
            module.with_config([settings](modules::mirror::config& config) {
                persistence::load_config(config, *settings);
            });
            module.activate();
        } else {
            module.deactivate();
            module.with_config([](auto& config) {
                config = modules::mirror::config::get_default();
            });
        }
    }

    const auto SHUTTER_CONTEXT = get_shutter_envir_context();
    // Load shutter modules.
    for (driver::channel_id channel :
         utils::as_iterable(driver::CHANNEL_RANGE_SHUTTER)) {
        auto driver    = _driver.get_shutter_cpld_driver(channel, factory);
        auto resources = modules::shutter::resources{
            .proxy = driver,
        };
        auto& module = _state.shutters[utils::channel_to_index(
            channel, driver::CHANNEL_RANGE_SHUTTER)];
        if (auto* settings = setup.get_shutter_settings(channel); settings) {
            module.with_config(
                [&SHUTTER_CONTEXT, settings](modules::shutter::config& config) {
                    persistence::load_config(config, *settings,
                                             SHUTTER_CONTEXT);
                },
                resources);
            module.activate(resources);
        } else {
            module.deactivate(resources);
            module.with_config_if_inactive([](auto& config) {
                config = modules::shutter::config::get_default();
            });
        }
    }

    bool success                = true;
    attached_subobject_type& rt = *_subobject;
    rt.connected_sn             = params.connected_sn;
    rt.connected_device         = connected_device;

    // Allows for any future code that wishes to reject attached_subobject
    // construction.
    if (success) {
        _driver.set_power_enabled(factory, true);

        if (_state.interlock_cable_enabled) {
            _driver.interlock_pin_configure_as_input();
        } else {
            _driver.interlock_pin_force_bridges_to_wake();
        }
    }

    return success;
}

void thread_local_object::wait_until_awoken(TickType_t sleep_duration) {
    // Waits for one of three conditions
    // 1. The task is notified through its ISR.
    // 2. The shutters need to be serviced soon.
    // 3. The shutter task's period service needs to occur.
    const TickType_t SHUTTER_DELTA = _shutter_next_service_delay;
    ulTaskNotifyTake(pdTRUE, std::min(sleep_duration, SHUTTER_DELTA));
}

void thread_local_object::service_idle(TickType_t NOW) {
    // When in idle mode, we may not even need to lock SPI.
    // Therefore, let's defer the spi lock.
    drivers::spi::handle_factory spi_factory{std::defer_lock};

    service_joystick(spi_factory);
    service_apt(spi_factory);
    _last_time_expensive_serviced = NOW;
}

void thread_local_object::service(TickType_t NOW) {
    using namespace drivers::io;

#if DEBUG
    // Assert that the subobject is correct.
    if (!std::holds_alternative<attached_subobject_type>(_state)) {
        for (;;);
    }
#endif
    attached_subobject_type& subobject = *_subobject;

    drivers::spi::handle_factory spi_factory{};

    service_interlock(spi_factory);

    const bool GPIO_HAS_INTERRUPT =
        _driver.query_interrupt_channels(spi_factory);
    // Note:  the GPIO register is read when clearing the interrupt
    // Therefore, the query actually reads the interrupts and the GPIO levels.

    // Interrupt pin was raised.
    if (GPIO_HAS_INTERRUPT) {
        // Service the interrupt.
        for (std::size_t idx = 0;
             idx < std::size(driver::CHANNEL_RANGE_SHUTTER); ++idx) {
            const driver::channel_id channel =
                utils::index_to_channel(idx, driver::CHANNEL_RANGE_SHUTTER);
            if (_driver.channel_has_interrupt(channel)) {
                _driver.clear_channel_interrupt(channel);
                if (auto& shutter_module = _state.shutters[idx];
                    shutter_module.is_active()) {
                    const bool LEVEL =
                        _driver.get_shutter_trigger_level(channel);

                    service_trigger_action(
                        shutter_module.get_state()->driver,
                        shutter_module.get_config().trigger_mode, LEVEL);
                }
            }
        }
    }

    // Services expensive services if they haven't been serviced in some time.
    // This typically ignores servicing on interrupt wakeups unless the
    // interrupt wakeups are saturating the loop.
    if (const TickType_t EXPENSIVE_SERVICING_PERIOD = _service_period - 1;
        NOW - _last_time_expensive_serviced >= EXPENSIVE_SERVICING_PERIOD) {
        service_joystick(spi_factory);
        service_apt(spi_factory);
        _last_time_expensive_serviced = NOW;
    }

    // Servicing the input/output mode of the TTL signals and the OUTPUT
    // direction.
    for (std::size_t i = 0; i < subobject.MIRRORS; ++i) {
        const driver::channel_id CHANNEL =
            utils::index_to_channel(i, driver::CHANNEL_RANGE_SIO);
        const driver::sio_modes_e MODE = _driver.get_sio_mode(CHANNEL);
        auto& module                   = _state.mirrors[i];
        if (!module.is_active()) {
            continue;
        }

        if (!module.get_state()->enabled) {
            // Updating the target state to prevent enabling the channel from
            // moving the device. Using this logic b/c disabled and ON have the
            // same output state.
            module.get_state()->target_level = MODE != driver::sio_modes_e::OFF;
        } else {
            const driver::sio_modes_e TARGET_MODE =
                (module.get_state()->target_level ? driver::sio_modes_e::ON
                                                  : driver::sio_modes_e::OFF);
            if (MODE != TARGET_MODE) {
                _driver.set_sio_mode(spi_factory, CHANNEL, TARGET_MODE);
            }
        }
    }

    for (std::size_t i = 0; i < _state.shutters.size(); ++i) {
        modules::shutter& mod = _state.shutters[i];
        // Skip empty drivers.
        if (!mod.is_active()) {
            continue;
        }

        const driver::channel_id CHANNEL =
            utils::index_to_channel(i, driver::CHANNEL_RANGE_SHUTTER);

        // Update the interrupt mode of the shutter's trigger.
        const drivers::io::interrupt_modes_e GPIO_INTERRUPT_MODE =
            _driver.get_shutter_interrupt_mode(CHANNEL);
        switch (mod.get_config().trigger_mode) {
        case trigger_modes_e::DISABLED:
            if (GPIO_INTERRUPT_MODE !=
                drivers::io::interrupt_modes_e::DISABLED) {
                _driver.set_shutter_interrupt_mode(
                    spi_factory, CHANNEL,
                    drivers::io::interrupt_modes_e::DISABLED);
            }
            break;

        default:
            if (GPIO_INTERRUPT_MODE !=
                drivers::io::interrupt_modes_e::ON_CHANGE) {
                _driver.set_shutter_interrupt_mode(
                    spi_factory, CHANNEL,
                    drivers::io::interrupt_modes_e::ON_CHANGE);
            }
            break;
        }

        auto proxy = _driver.get_shutter_cpld_driver(CHANNEL, spi_factory);
        mod.get_state()->driver.service(proxy, NOW);
    }

    _shutter_next_service_delay = get_time_until_next_shutter_service(NOW);
}

void thread_local_object::post_service_cleanup() {
    drivers::spi::handle_factory factory;

    _driver.set_power_enabled(factory, false);
    _driver.interlock_pin_force_bridges_to_sleep();

    // Force all modules to deactivate.
    for (auto& module : _state.mirrors) {
        module.deactivate();
    }
    for (driver::channel_id channel :
         utils::as_iterable(driver::CHANNEL_RANGE_SHUTTER)) {
        auto driver    = _driver.get_shutter_cpld_driver(channel, factory);
        auto resources = modules::shutter::resources{
            .proxy = driver,
        };
        auto& module = _state.shutters[utils::channel_to_index(
            channel, driver::CHANNEL_RANGE_SHUTTER)];
        module.deactivate(resources);
    }
}

void thread_local_object::state_t::open_shutter(driver::channel_id channel) {
    configASSERT(
        utils::is_channel_in_range(channel, driver::CHANNEL_RANGE_SHUTTER));
    auto& shutter = shutters[utils::channel_to_index(
        channel, driver::CHANNEL_RANGE_SHUTTER)];
    const bool USB_CONTROL_ENABLED =
        !shutter_ttl_control_disables_usb_control ||
        shutter.get_config().trigger_mode == trigger_modes_e::DISABLED;
    if (shutter.is_active() && shutter.get_state()->enabled &&
        USB_CONTROL_ENABLED) {
        shutter.get_state()->driver.async_open();
    }
}
void thread_local_object::state_t::close_shutter(driver::channel_id channel) {
    configASSERT(
        utils::is_channel_in_range(channel, driver::CHANNEL_RANGE_SHUTTER));
    auto& shutter = shutters[utils::channel_to_index(
        channel, driver::CHANNEL_RANGE_SHUTTER)];
    const bool USB_CONTROL_ENABLED =
        !shutter_ttl_control_disables_usb_control ||
        shutter.get_config().trigger_mode == trigger_modes_e::DISABLED;
    if (shutter.is_active() && shutter.get_state()->enabled &&
        USB_CONTROL_ENABLED) {
        shutter.get_state()->driver.async_close();
    }
}
void thread_local_object::state_t::toggle_shutter(driver::channel_id channel) {
    configASSERT(
        utils::is_channel_in_range(channel, driver::CHANNEL_RANGE_SHUTTER));
    auto& shutter = shutters[utils::channel_to_index(
        channel, driver::CHANNEL_RANGE_SHUTTER)];
    const bool USB_CONTROL_ENABLED =
        !shutter_ttl_control_disables_usb_control ||
        shutter.get_config().trigger_mode == trigger_modes_e::DISABLED;
    if (shutter.is_active() && shutter.get_state()->enabled &&
        USB_CONTROL_ENABLED) {
        shutter.get_state()->driver.async_toggle();
    }
}

thread_local_object::thread_local_object(const initial_parameter_type& params,
                                         drivers::spi::handle_factory& factory)
    : _driver(
          driver::create(factory, params.slot, create_driver_options(params)))
    , _interlock_state(interlock_servicing_states_e::IGNORED_BRIDGES_SLEEPING)
    , _joystick_in_queue(*pipeline_hid_in().create_queue(params.slot))
    , _cdc_queue(*service::itc::pipeline_cdc().create_queue(
          static_cast<asf_destination_ids>((int)params.slot + (int)SLOT_1_ID)))
    , _service_period(params.service_period)
    , _watchdog_service_timeout(params.watchdog_service_timeout)
    , _watchdog_configuring_timeout(params.watchdog_configuring_timeout)
    , _last_time_expensive_serviced(0)
// , _card_type(params.card_type)
{}

void thread_local_object::service_interlock(
    drivers::spi::handle_factory& factory) {
    // TODO
    switch (_interlock_state) {
        using enum interlock_servicing_states_e;
    // case IGNORED_BRIDGES_SLEEPING <-- Needs to be handled
    // in the setup process
    case WAITING_FOR_ACTIVATION:
        if (_driver.is_interlock_pin_engaged()) {
            // _interlock_state =
            // ACTIVATED_UNHANDLED_CONSEQUENCES;

            // if (consequences_require_driven_bridges()) {
            //     drive_interlock_wake_shutter_chip();
            //     _interlock_state =
            //     HANDLING_CONSEQUENCES_WITH_AWOKEN_BRIDGES;
            //     dispatch_consequence_mitigation();
            // } else {
            //     _interlock_state =
            //     ACTIVATED_HANDLED_CONSEQUENCES;
            // }
        }
        break;
    // case ACTIVATED_UNHANDLED_CONSEQUENCES <-- State is
    // transitioned into and handled in waiting for
    // activation
    case HANDLING_CONSEQUENCES_WITH_AWOKEN_BRIDGES:
        // if (consequences_have_been_resolved()) {
        //     release_interlock_to_readable();
        //     _interlock_state =
        //     ACTIVATED_HANDLED_CONSEQUENCES;
        // }
        break;
    case ACTIVATED_HANDLED_CONSEQUENCES:
        if (!_driver.is_interlock_pin_engaged()) {
            _interlock_state = WAITING_FOR_ACTIVATION;
        }
        break;
    }
}

void thread_local_object::service_joystick(drivers::spi::handle_factory& spi) {
    auto service_in = [&](const ::service::itc::message::hid_in_message& msg) {
        switch (msg.usage_page) {
        case HID_USAGE_PAGE_BUTTON:
            switch (msg.mode) {
            case CTL_BTN_POS_TOGGLE:
                for (std::size_t idx = 0;
                     idx < std::size(driver::CHANNEL_RANGE_SHUTTER); ++idx) {
                    if ((msg.destination_channel_bitset & (1 << idx)) != 0) {
                        _state.toggle_shutter(utils::index_to_channel(
                            idx, driver::CHANNEL_RANGE_SHUTTER));
                    }
                }
                break;

            default:
                break;
            }
            break;
        default:
            break;
        }
    };
    auto service_out = [&]() {
        // Send position data to USB data out queue.
        auto msg =
            ::service::itc::message::hid_out_message::get_default(get_slot());

        msg.on_stored_position_bitset = 0;

        // Set the position of each shutter channel.
        for (std::size_t idx = 0;
             idx < std::size(driver::CHANNEL_RANGE_SHUTTER); ++idx) {
            auto& mod = _state.shutters[idx];
            if (mod.is_active()) {
                if (mod.get_state()->driver.current_state() ==
                    cards::shutter::controller::states_e::OPEN) {
                    msg.on_stored_position_bitset |= (1 << idx);
                }
            }
        }

        pipeline_hid_out().broadcast(msg);
    };

    ::service::itc::message::hid_in_message msg;
    while (_joystick_in_queue.try_pop(msg, 0)) {
        service_in(msg);
    }
    service_out();
}

bool thread_local_object::parse_usb_cmd(
    drivers::spi::handle_factory& spi,
    drivers::usb::apt_basic_command& command) {
    bool send_response = false;

    using namespace drivers::usb;
    using namespace drivers::apt;

    auto check_slider_io_parsed = [](const auto& maybe) {
        return maybe && utils::is_channel_in_range(
                            static_cast<driver::channel_id>(maybe->channel),
                            driver::CHANNEL_RANGE_SIO);
    };

    auto check_shutter_parsed = [](const auto& maybe) {
        return maybe && utils::is_channel_in_range(
                            static_cast<driver::channel_id>(maybe->channel),
                            driver::CHANNEL_RANGE_SHUTTER);
    };

    switch (command.command()) {
    case MGMSG_MCM_ERASE_DEVICE_CONFIGURATION:
        with_persistence_controller(
            [](persistence::persistence_controller<
                persistence::layout_25lc1024>& ctrler) {
                ctrler.set_card_settings(
                    std::optional<persistence::card_settings>());

                for (driver::channel_id channel :
                     utils::as_iterable(driver::CHANNEL_RANGE_SIO)) {
                    ctrler.set_sio_settings(
                        std::optional<persistence::sio_settings>(), channel);
                }

                for (driver::channel_id channel :
                     utils::as_iterable(driver::CHANNEL_RANGE_SHUTTER)) {
                    ctrler.set_shutter_settings(
                        std::optional<persistence::shutter_settings>(),
                        channel);
                }
            },
            spi);
        break;

    case mot_eepromparams::COMMAND_SET: {
        auto maybe = apt_struct_set<mot_eepromparams>(command);

        if (maybe) {
            const driver::channel_id CHANNEL =
                static_cast<driver::channel_id>(maybe->params_as_channel());
            const uint64_t SN = _subobject ? _subobject->connected_sn
                                           : OW_SERIAL_NUMBER_WILDCARD;

            switch (maybe->msg_id) {
            case mcm_shutter_params::COMMAND_SET:
                if (utils::is_channel_in_range(CHANNEL,
                                               driver::CHANNEL_RANGE_SHUTTER)) {
                    const modules::shutter::config& CONFIG =
                        _state
                            .shutters[utils::channel_to_index(
                                CHANNEL, driver::CHANNEL_RANGE_SHUTTER)]
                            .get_config();
                    with_persistence_controller(
                        [&CONFIG, &SN, CHANNEL,
                         CONTEXT = get_shutter_envir_context()](
                            persistence::persistence_controller<
                                persistence::layout_25lc1024>& ctrler) {
                            std::optional<persistence::shutter_settings> opt(
                                std::in_place);
                            opt->saved_serial_number = SN;
                            persistence::store_config(*opt, CONFIG, CONTEXT);
                            ctrler.set_shutter_settings(opt, CHANNEL);
                        },
                        spi);
                }
                break;
            case mcm_mirror_params::COMMAND_SET:
                if (utils::is_channel_in_range(CHANNEL,
                                               driver::CHANNEL_RANGE_SIO)) {
                    const modules::mirror::config& CONFIG =
                        _state
                            .mirrors[utils::channel_to_index(
                                CHANNEL, driver::CHANNEL_RANGE_SIO)]
                            .get_config();
                    with_persistence_controller(
                        [&CONFIG, &SN,
                         CHANNEL](persistence::persistence_controller<
                                  persistence::layout_25lc1024>& ctrler) {
                            std::optional<persistence::sio_settings> opt(
                                std::in_place);
                            opt->saved_serial_number = SN;
                            persistence::store_config(*opt, CONFIG);
                            ctrler.set_sio_settings(opt, CHANNEL);
                        },
                        spi);
                }
                break;
            }
        }
    } break;

    case mod_chanenablestate::COMMAND_SET: {
        auto maybe = apt_struct_set<mod_chanenablestate>(command);

        if (maybe) {
            const driver::channel_id CHAN =
                static_cast<driver::channel_id>(maybe->channel);
            if (utils::is_channel_in_range(CHAN, driver::CHANNEL_RANGE_SIO)) {
                auto& mod = _state.mirrors[utils::channel_to_index(
                    CHAN, driver::CHANNEL_RANGE_SIO)];
                modules::apt_handler(mod, *maybe);
            } else if (utils::is_channel_in_range(
                           CHAN, driver::CHANNEL_RANGE_SHUTTER)) {
                auto& mod = _state.shutters[utils::channel_to_index(
                    CHAN, driver::CHANNEL_RANGE_SHUTTER)];
                modules::apt_handler(mod, *maybe);
            }
        }
    } break;

    case mod_chanenablestate::COMMAND_REQ: {
        auto maybe = apt_struct_req<mod_chanenablestate>(command);

        if (maybe) {
            const driver::channel_id CHAN =
                static_cast<driver::channel_id>(maybe->channel);

            mod_chanenablestate::payload_type payload;
            payload.channel = maybe->channel;

            if (utils::is_channel_in_range(CHAN, driver::CHANNEL_RANGE_SIO)) {
                auto& mod = _state.mirrors[utils::channel_to_index(
                    CHAN, driver::CHANNEL_RANGE_SIO)];
                modules::apt_handler(mod, *maybe, payload);
            } else if (utils::is_channel_in_range(
                           CHAN, driver::CHANNEL_RANGE_SHUTTER)) {
                auto& mod = _state.shutters[utils::channel_to_index(
                    CHAN, driver::CHANNEL_RANGE_SHUTTER)];
                modules::apt_handler(mod, *maybe, payload);
            } else {
                payload.is_enabled = false;
            }
            apt_struct_get<mod_chanenablestate>(_response, payload);
            send_response = true;
        }
    } break;

    case mcm_mirror_state::COMMAND_SET: {
        // Sets up the target state but does not immediately
        // actuate.
        auto maybe = apt_struct_set<mcm_mirror_state>(command);

        if (check_slider_io_parsed(maybe)) {
            auto& mod = _state.mirrors[utils::channel_to_index(
                static_cast<driver::channel_id>(maybe->channel),
                driver::CHANNEL_RANGE_SIO)];
            modules::apt_handler(mod, *maybe);
        }
    } break;

    case mcm_mirror_state::COMMAND_REQ: {
        // Reads the current state -- even if disabled
        // (assumes TRUE to due to pull-up)
        auto maybe = apt_struct_req<mcm_mirror_state>(command);

        if (check_slider_io_parsed(maybe)) {
            using s = mirror_types::states_e;
            apt_struct_get<mcm_mirror_state>(
                _response, mcm_mirror_state::payload_type{
                               .channel = maybe->channel,
                               .state = _driver.get_sio_mode(maybe->channel) ==
                                                driver::sio_modes_e::OFF
                                            ? s::OUT
                                            : s::IN,
                           });
            send_response = true;
        }
    } break;

    case mcm_mirror_params::COMMAND_REQ: {
        auto maybe = apt_struct_req<mcm_mirror_params>(command);

        if (check_slider_io_parsed(maybe)) {
            using s = mirror_types::states_e;
            apt_struct_get<mcm_mirror_params>(
                _response,
                mcm_mirror_params::payload_type{
                    .initial_state = s::IN,  // Due to pull-up resistor
                    .mode          = mirror_types::modes_e::SIGNAL,
                    .channel       = maybe->channel,
                    .pwm_low_val   = 0,
                    .pwm_high_val  = 0,
                });
            send_response = true;
        }
    } break;

    case mcm_shutter_params::COMMAND_SET: {
        auto maybe = apt_struct_set<mcm_shutter_params>(command);

        if (check_shutter_parsed(maybe)) {
            // Convert the persumed 1900 period to the actual period.
            maybe->duty_cycle_pulse =
                drivers::cpld::shutter_module::get_equivalent_duty(
                    (drivers::cpld::shutter_module::duty_type)
                        maybe->duty_cycle_pulse,
                    1900, _driver.get_pwm_period());
            maybe->duty_cycle_hold =
                drivers::cpld::shutter_module::get_equivalent_duty(
                    (drivers::cpld::shutter_module::duty_type)
                        maybe->duty_cycle_hold,
                    1900, _driver.get_pwm_period());
            const std::size_t INDEX = utils::channel_to_index(
                static_cast<driver::channel_id>(maybe->channel),
                driver::CHANNEL_RANGE_SHUTTER);
            auto& mod  = _state.shutters[INDEX];
            auto proxy = _driver.get_shutter_cpld_driver(maybe->channel, spi);

            modules::shutter::resources resources = {
                .proxy = proxy,
            };
            // Block activations while the subobject is empty
            // which implies no connected device.
            modules::apt_handler(mod, *maybe, resources,
                                 !_subobject.has_value());

            if (mod.is_active()) {
                service_trigger_action(
                    mod.get_state()->driver, mod.get_config().trigger_mode,
                    _driver.get_shutter_trigger_level(maybe->channel));
            }
        }
    } break;

    case mcm_shutter_params::COMMAND_REQ: {
        auto maybe = apt_struct_req<mcm_shutter_params>(command);

        if (maybe) {
            mcm_shutter_params::payload_type response;
            if (check_shutter_parsed(maybe)) {
                const std::size_t INDEX = utils::channel_to_index(
                    static_cast<driver::channel_id>(maybe->channel),
                    driver::CHANNEL_RANGE_SHUTTER);
                auto& mod = _state.shutters[INDEX];
                modules::apt_handler(mod, *maybe, response);
            } else {
                response.initial_state = shutter_types::states_e::UNKNOWN;
                response.type          = shutter_types::types_e::NONE;
                response.external_trigger_mode =
                    shutter_types::trigger_modes_e::DISABLED;
                response.duty_cycle_pulse = 0;
                response.duty_cycle_hold  = 0;
                response.channel          = maybe->channel;
                response.on_time =
                    utils::time::to_duration<decltype(response.on_time)>(0);
                response.optional_data =
                    mcm_shutter_params::payload_type::variant_len19{
                        .actuation_holdoff_time =
                            utils::time::hundreds_of_us<uint32_t>(0),
                        .reverse_open_level = false,
                    };
            }
            // Convert the actual period to the presumed 1900 period.
            response.duty_cycle_pulse =
                drivers::cpld::shutter_module::get_equivalent_duty(
                    (drivers::cpld::shutter_module::duty_type)
                        response.duty_cycle_pulse,
                    _driver.get_pwm_period(), 1900);
            response.duty_cycle_hold =
                drivers::cpld::shutter_module::get_equivalent_duty(
                    (drivers::cpld::shutter_module::duty_type)
                        response.duty_cycle_hold,
                    _driver.get_pwm_period(), 1900);

            apt_struct_get<mcm_shutter_params>(_response, response);
            static_assert(
                variable_span_serializable<mcm_shutter_params::payload_type>);
            send_response = true;
        }
    } break;

        // (sbenish) These messages have not been used so they have been
        // omitted for the time being.
        // case mcm_shuttercomp_params::COMMAND_SET: {
        //     auto maybe = apt_struct_set<mcm_shuttercomp_params>(command);
        //
        //     if (check_shutter_parsed(maybe)) {
        //         const std::size_t INDEX = utils::channel_to_index(
        //             static_cast<driver::channel_id>(maybe->channel),
        //             driver::CHANNEL_RANGE_SHUTTER);
        //         auto& mod  = _state.shutters[INDEX];
        //         auto proxy = _driver.get_shutter_cpld_driver(maybe->channel,
        //         spi);
        //
        //         modules::shutter::resources resources = {
        //             .proxy = proxy,
        //         };
        //         // Block activations while the subobject is empty
        //         // which implies no connected device.
        //         modules::apt_handler(mod, *maybe, resources,
        //                              !_subobject.has_value());
        //     }
        // } break;
        //
        // case mcm_shuttercomp_params::COMMAND_REQ: {
        //     auto maybe = apt_struct_req<mcm_shuttercomp_params>(command);
        //
        //     if (maybe) {
        //         mcm_shuttercomp_params::payload_type response;
        //         if (check_shutter_parsed(maybe)) {
        //             const std::size_t INDEX = utils::channel_to_index(
        //                 static_cast<driver::channel_id>(maybe->channel),
        //                 driver::CHANNEL_RANGE_SHUTTER);
        //             auto& mod = _state.shutters[INDEX];
        //             modules::apt_handler(mod, *maybe, response);
        //         } else {
        //             response.open_actuation_voltage  = 0;
        //             response.open_actuation_time     = 0;
        //             response.opened_hold_voltage     = 0;
        //             response.close_actuation_voltage = 0;
        //             response.close_actuation_time    = 0;
        //             response.closed_hold_voltage     = 0;
        //             response.channel                 = maybe->channel;
        //             response.power_up_state =
        //             shutter_types::states_e::UNKNOWN; response.trigger_mode =
        //                 shutter_types::trigger_modes_e::DISABLED;
        //         }
        //         apt_struct_get<mcm_shuttercomp_params>(_response, response);
        //         send_response = true;
        //     }
        // } break;

    case mcm_interlock_state::COMMAND_REQ: /* 0x4067*/
        apt_struct_get<mcm_interlock_state>(_response,
                                            _driver.is_interlock_pin_engaged());
        send_response = true;
        break;

    case mot_solenoid_state::COMMAND_SET: { /* 0x04CB*/
        auto maybe = apt_struct_set<mot_solenoid_state>(command);

        if (check_shutter_parsed(maybe)) {
            const auto CHANNEL =
                static_cast<driver::channel_id>(maybe->channel);
            auto& mod = _state.shutters[utils::channel_to_index(
                CHANNEL, driver::CHANNEL_RANGE_SHUTTER)];
            if (!_state.shutter_ttl_control_disables_usb_control) {
                modules::apt_handler(mod, *maybe);
            }
        }
    } break;
    case mot_solenoid_state::COMMAND_REQ: { /* 0x04CC*/
        auto maybe = apt_struct_req<mot_solenoid_state>(command);

        if (check_shutter_parsed(maybe)) {
            mot_solenoid_state::payload_type payload;

            auto& mod = _state.shutters[utils::channel_to_index(
                static_cast<driver::channel_id>(maybe->channel),
                driver::CHANNEL_RANGE_SHUTTER)];
            modules::apt_handler(mod, *maybe, payload);
            apt_struct_get<mot_solenoid_state>(_response, payload);
            send_response = true;
        }
    } break;

    case mcm_pwm_period::COMMAND_SET:
        if (auto maybe = apt_struct_set<mcm_pwm_period>(command); maybe) {
            maybe->current =
                drivers::cpld::shutter_module::clamp_period(maybe->current);

            // Disable all shutter controllers and prepare
            // for runtime reinitialization.
            std::array<bool, std::size(driver::CHANNEL_RANGE_SHUTTER)>
                was_active;
            for (std::size_t i = 0; i < _state.shutters.size(); ++i) {
                auto& module = _state.shutters[i];
                auto proxy   = _driver.get_shutter_cpld_driver(
                    utils::index_to_channel(i, _driver.CHANNEL_RANGE_SHUTTER),
                    spi);
                auto resources = modules::shutter::resources{
                    .proxy = proxy,
                };
                was_active[i] = module.is_active();
                module.deactivate(resources);
                module.with_config(
                    [VALUE = maybe->current](modules::shutter::config& cfg) {
                        cfg.propagate_period_change(VALUE);
                    },
                    resources);
            }

            _driver.set_pwm_period(spi, maybe->current);

            // Reboot any disabled shutters.
            for (std::size_t i = 0; i < _state.shutters.size(); ++i) {
                if (!was_active[i]) {
                    continue;
                }

                auto& module = _state.shutters[i];
                auto proxy   = _driver.get_shutter_cpld_driver(
                    utils::index_to_channel(i, _driver.CHANNEL_RANGE_SHUTTER),
                    spi);
                auto resources = modules::shutter::resources{
                    .proxy = proxy,
                };
                module.activate(resources);
            }
        }
        break;

    case mcm_pwm_period::COMMAND_REQ:
        apt_struct_get<mcm_pwm_period>(
            _response,
            mcm_pwm_period::payload_type{
                .current = _driver.get_pwm_period(),
                .get_only =
                    {
                        .min = drivers::cpld::shutter_module::MINIMUM_PERIOD,
                        .max = drivers::cpld::shutter_module::MAXIMUM_PERIOD,
                    },
            });
        send_response = true;
        break;

    case mcm_shutter_trigger::COMMAND_SET:
        if (auto maybe = apt_struct_set<mcm_shutter_trigger>(command); maybe) {
            if (check_shutter_parsed(maybe)) {
                const auto CHANNEL =
                    static_cast<driver::channel_id>(maybe->channel);
                auto& mod      = _state.shutters[utils::channel_to_index(
                    CHANNEL, driver::CHANNEL_RANGE_SHUTTER)];
                auto proxy     = _driver.get_shutter_cpld_driver(CHANNEL, spi);
                auto resources = modules::shutter::resources{
                    .proxy = proxy,
                };

                modules::apt_handler(mod, *maybe, resources);

                // Do a level trigger here.
                const bool level = _driver.get_shutter_trigger_level(CHANNEL);
                switch (maybe->mode) {
                default:
                    break;

                case trigger_modes_e::ENABLED:
                    if (level) {
                        _state.open_shutter(CHANNEL);
                    } else {
                        _state.close_shutter(CHANNEL);
                    }
                    break;

                case trigger_modes_e::ENABLED_INVERTED:
                    if (!level) {
                        _state.open_shutter(CHANNEL);
                    } else {
                        _state.close_shutter(CHANNEL);
                    }
                    break;
                }
            }
        }
        break;

    case mcm_shutter_trigger::COMMAND_REQ:
        if (auto maybe = apt_struct_req<mcm_shutter_trigger>(command); maybe) {
            mcm_shutter_trigger::payload_type payload;
            payload.channel = maybe->channel;

            if (check_shutter_parsed(maybe)) {
                const auto CHANNEL =
                    static_cast<driver::channel_id>(maybe->channel);
                auto& mod = _state.shutters[utils::channel_to_index(
                    CHANNEL, driver::CHANNEL_RANGE_SHUTTER)];
                modules::apt_handler(mod, *maybe, payload);
            } else {
                payload.mode =
                    drivers::apt::shutter_types::trigger_modes_e::DISABLED;
            }

            apt_struct_get<mcm_shutter_trigger>(_response, payload);
            send_response = true;
        }
        break;
    }

    return send_response;
}
void thread_local_object::service_apt(drivers::spi::handle_factory& factory) {
    // (sbenish):  The USB_Slave_Message and response buffer
    // are both 440 bytes.
    USB_Slave_Message msg_elem;
    while (_cdc_queue.try_pop(msg_elem)) {
        drivers::usb::apt_basic_command cmd = (msg_elem);
        const bool HAS_MSG                  = parse_usb_cmd(factory, cmd);
        if (HAS_MSG) {
            drivers::usb::apt_command_factory::commit(cmd, _response);
        }
    }
}

void thread_local_object::service_trigger_action(
    cards::shutter::controller& controller, trigger_modes_e mode, bool level) {
    switch (mode) {
    case trigger_modes_e::ENABLED:
        if (level) {
            controller.async_open();
        } else {
            controller.async_close();
        }
        break;

    case trigger_modes_e::ENABLED_INVERTED:
        if (level) {
            controller.async_close();
        } else {
            controller.async_open();
        }
        break;

    case trigger_modes_e::DISABLED:
    default:
        break;
    };
}

TickType_t thread_local_object::get_time_until_next_shutter_service(
    TickType_t now) const {
    return std::ranges::min(
        _state.shutters |
        std::views::transform([now](
                                  const modules::shutter& value) -> TickType_t {
            return !value.is_active()
                       ? portMAX_DELAY
                       : value.get_state()->driver.get_remaining_actuation_time(
                             now);
        }));
}

static void raise_smio_interrupt_from_isr(
    slot_nums slot, portBASE_TYPE& higher_priority_task_awoken) {
    // The interrupt flags are now handled on a pin-by-pin
    // basis.  The interrupt SMIO flag is not required.
    // slots[slot].interrupt_flag_smio = 1;
    vTaskNotifyGiveFromISR(xSlotHandle[slot], &higher_priority_task_awoken);
}

static void on_trigger_interrupt(uint8_t slot, void*) {
    portBASE_TYPE higher_priority_task_awoken;

    if (xSlotHandle[slot] == NULL) {
        return;
    }

    // (sbenish)  2024-06-12 16:31 ET
    // The worst case latency between the signal level changing
    // and the shutter's driver moving was 140 µs.
    raise_smio_interrupt_from_isr(static_cast<slot_nums>(slot),
                                  higher_priority_task_awoken);
    portYIELD_FROM_ISR(higher_priority_task_awoken);
}

static driver::creation_options create_driver_options(
    const thread_local_object::initial_parameter_type& params) noexcept {
    auto options =
        cards::flipper_shutter::driver::creation_options::get_default();

    // RevA and later has inverted enable pin logic.
    options.invert_power_pin =
        params.card_type != slot_types::MCM_Flipper_Shutter;
    return options;
}

// EOF
