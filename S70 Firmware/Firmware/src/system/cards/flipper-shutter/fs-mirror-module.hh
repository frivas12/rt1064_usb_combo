#pragma once

#include "cpld-shutter-driver.hh"
#include "flipper-shutter-persistence.hh"
#include "mcm_mirror_params.hh"
#include "mcm_mirror_state.hh"
#include "mirror-types.hh"
#include "mod_chanenablestate.hh"

/**
 * Configuration and state module for the flipper-shutter's mirror
 * channels.
 */
namespace cards::flipper_shutter {
namespace modules {
class mirror {
   public:
    struct config {
        bool is_logic_high_in;
        bool default_level;

        static constexpr config get_default() noexcept {
            return {
                .is_logic_high_in = true,
                .default_level    = true,
            };
        }
    };

    struct state {
        bool target_level;
        bool enabled;

        state(const config& cfg);

        /// \brief Sets the target level with inversion logic from config.
        constexpr void set_position(
            const config& cfg, drivers::apt::mirror_types::states_e state) {
            target_level =
                (state == drivers::apt::mirror_types::states_e::IN) ==
                cfg.is_logic_high_in;
        }

        /// \brief Gets the position with inversion logic from config.
        constexpr drivers::apt::mirror_types::states_e get_position(
            const config& cfg) const {
            return (target_level == cfg.is_logic_high_in)
                       ? drivers::apt::mirror_types::states_e::IN
                       : drivers::apt::mirror_types::states_e::OUT;
        }
    };

    inline const config& get_config() const noexcept { return _config; }
    inline state* get_state() noexcept { return _state ? &*_state : nullptr; }
    inline const state* get_state() const noexcept {
        return _state ? &*_state : nullptr;
    };

    /// \brief If the module is active (if it has state).
    inline bool is_active() const noexcept { return _state.has_value(); }

    /// \brief Forces the module to become active.
    /// If the module is already active, it will deactive the module before
    /// re-activating.
    void activate() noexcept;

    /// \brief Forces the module to become inactive.
    void deactivate() noexcept;

    /**
     * Allows visitors to mutate the config and, if active, reloads the state
     * object.
     * \param  visitor The function that mutates the config.
     * \param  proxy   The cpld shutter-module proxy driving this channel.
     * \return The return of the visitor (if any).
     */
    auto with_config(std::invocable<config&> auto&& visitor) noexcept {
        if constexpr (std::is_void_v<std::invoke_result_t<
                          std::decay_t<decltype(visitor)>, config&>>) {
            visitor(_config);
            update_state_from_config();
        } else {
            auto rt = visitor(_config);
            update_state_from_config();
            return rt;
        }
    }

   private:
    std::optional<state> _state = {};
    config _config              = config::get_default();

    void update_state_from_config() noexcept;
};

bool apt_handler(
    mirror& module,
    const drivers::apt::mod_chanenablestate::payload_type& command);
bool apt_handler(const mirror& module,
                 const drivers::apt::mod_chanenablestate::request_type& command,
                 drivers::apt::mod_chanenablestate::payload_type& response);

bool apt_handler(mirror& module,
                 const drivers::apt::mcm_mirror_state::payload_type& command);
bool apt_handler(const mirror& module,
                 const drivers::apt::mcm_mirror_state::request_type& command,
                 drivers::apt::mcm_mirror_state::payload_type& response);

bool apt_handler(const mirror& module,
                 const drivers::apt::mcm_mirror_params::request_type& command,
                 drivers::apt::mcm_mirror_params::payload_type& response);

}  // namespace modules
namespace persistence {
/// \brief Loads the persistence shutter settings into the module's config.
void load_config(modules::mirror::config& dest,
                 const sio_settings& src) noexcept;

/// \brief Stores the config into shutter settings object for serialization.
void store_config(sio_settings& dest,
                  const modules::mirror::config& src) noexcept;
}  // namespace persistence

}  // namespace cards::flipper_shutter
