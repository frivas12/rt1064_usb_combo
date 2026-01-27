#include "flipper-shutter-query.hh"
#include "channels.hh"
#include <expected>

using namespace cards::flipper_shutter;
using namespace cards::flipper_shutter::persistence;

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
const card_settings *setup_query::get_card_settings() const {
    return _card_settings ? &(*_card_settings) : nullptr;
}
const sio_settings *
setup_query::get_sio_settings(driver::channel_id channel) const {
    if (!utils::is_channel_in_range(channel, driver::CHANNEL_RANGE_SIO)) {
        return nullptr;
    }

    auto& target =
        _sios[utils::channel_to_index(channel, driver::CHANNEL_RANGE_SIO)];
    return target ? &(*target) : nullptr;
}

const shutter_settings *
setup_query::get_shutter_settings(driver::channel_id channel) const {
    if (!utils::is_channel_in_range(channel, driver::CHANNEL_RANGE_SHUTTER)) {
        return nullptr;
    }

    auto& target = _shutters[utils::channel_to_index(
        channel, driver::CHANNEL_RANGE_SHUTTER)];
    return target ? &(*target) : nullptr;
}

void setup_query::add(const persistence::card_settings &value) {
    _card_settings = value;
}

void setup_query::add(const persistence::sio_settings &value,
                          driver::channel_id channel) {
    if (!utils::is_channel_in_range(channel, driver::CHANNEL_RANGE_SIO)) {
        return;
    }

    _sios[utils::channel_to_index(channel, driver::CHANNEL_RANGE_SIO)] = value;
}

void setup_query::add(const persistence::shutter_settings &value,
                          driver::channel_id channel) {
    if (!utils::is_channel_in_range(channel, driver::CHANNEL_RANGE_SHUTTER)) {
        return;
    }

    _shutters[utils::channel_to_index(channel, driver::CHANNEL_RANGE_SHUTTER)] =
        value;
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

// EOF
