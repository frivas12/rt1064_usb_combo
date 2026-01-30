#include "stepper.details.hh"

#include <algorithm>

#include "mcm_speed_limit.hh"
#include "stepper.h"

using namespace cards::stepper;
using namespace drivers::apt;

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

drivers::apt::mcm_speed_limit::payload_type cards::stepper::apt_handler(
    const drivers::apt::mcm_speed_limit::request_type& request,
    const Stepper_info& stepper) {
    mcm_speed_limit::payload_type rt;
    rt.channel     = request.channel;
    rt.type_bitset = request.type_bitset;
    rt.speed_limit = 0;
    if (request.channel == stepper.slot) {
        if (request.one_channel_selected()) {
            switch (request.get_movement_type()) {
            case mcm_speed_limit::movement_type_e::ABSOLUTE:
                rt.speed_limit = stepper_get_speed_channel_value(
                    &stepper, STEPPER_SPEED_CHANNEL_ABSOLUTE);
                break;

            case mcm_speed_limit::movement_type_e::RELATIVE:
                rt.speed_limit = stepper_get_speed_channel_value(
                    &stepper, STEPPER_SPEED_CHANNEL_RELATIVE);
                break;

            case mcm_speed_limit::movement_type_e::JOG:
                rt.speed_limit = stepper_get_speed_channel_value(
                    &stepper, STEPPER_SPEED_CHANNEL_JOG);
                break;

            case mcm_speed_limit::movement_type_e::VELOCITY:
                rt.speed_limit = stepper_get_speed_channel_value(
                    &stepper, STEPPER_SPEED_CHANNEL_VELOCITY);
                break;

            case mcm_speed_limit::movement_type_e::HOMING:
                rt.speed_limit = stepper_get_speed_channel_value(
                    &stepper, STEPPER_SPEED_CHANNEL_HOMING);
                break;

            default:
                break;
            }
        } else if (request.is_hardware_channel_selected()) {
            rt.speed_limit = stepper_get_speed_channel_value(
                &stepper, STEPPER_SPEED_CHANNEL_UNBOUND);
        }
    }

    return rt;
}

void cards::stepper::apt_handler(
    const drivers::apt::mcm_speed_limit::payload_type& set,
    Stepper_info& stepper) {
    if (set.channel != (channel_t)stepper.slot) {
        return;
    }

    for (mcm_speed_limit::movement_type_e type :
         mcm_speed_limit::MOVEMENT_TYPES) {
        if (set.is_movement_type_set(type)) {
            // Set a minimum of 1 -- otherwise the logic breaks.
            const uint32_t SPEED_LIMIT = std::clamp<uint32_t>(
                set.speed_limit, 1,
                stepper_get_speed_channel_value(&stepper,
                                                STEPPER_SPEED_CHANNEL_UNBOUND));

            switch (type) {
            case mcm_speed_limit::movement_type_e::ABSOLUTE:
                stepper_set_speed_channel_value(
                    &stepper, STEPPER_SPEED_CHANNEL_ABSOLUTE, SPEED_LIMIT);
                break;

            case mcm_speed_limit::movement_type_e::RELATIVE:
                stepper_set_speed_channel_value(
                    &stepper, STEPPER_SPEED_CHANNEL_RELATIVE, SPEED_LIMIT);
                break;

            case mcm_speed_limit::movement_type_e::JOG:
                stepper_set_speed_channel_value(
                    &stepper, STEPPER_SPEED_CHANNEL_JOG, SPEED_LIMIT);
                break;

            case mcm_speed_limit::movement_type_e::VELOCITY:
                stepper_set_speed_channel_value(
                    &stepper, STEPPER_SPEED_CHANNEL_VELOCITY, SPEED_LIMIT);
                break;

            case mcm_speed_limit::movement_type_e::HOMING:
                stepper_set_speed_channel_value(
                    &stepper, STEPPER_SPEED_CHANNEL_HOMING, SPEED_LIMIT);
                break;

            default:
                break;
            }
        }
    }
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
