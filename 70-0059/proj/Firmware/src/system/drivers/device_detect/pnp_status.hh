// pnp_status.h

/**************************************************************************//**
 * \file pnp_status.h
 * \author Sean Benish
* \brief Holds the information needed for reporting on the status of the
 * Plug-and-Play systems.
 *****************************************************************************/

#ifndef SRC_SYSTEM_DRIVERS_DEVICE_DETECT_PNP_STATUS_HH_
#define SRC_SYSTEM_DRIVERS_DEVICE_DETECT_PNP_STATUS_HH_

#include "pnp_status.h"

/*****************************************************************************
 * Defines
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/

// Enum with all of the flags in the PnP status field.
namespace pnp_status
{
    constexpr pnp_status_t NO_ERRORS = 0;
    constexpr pnp_status_t NO_DEVICE_CONNECTED = 1 << 0;
    constexpr pnp_status_t GENERAL_OW_ERROR = 1 << 1;
    constexpr pnp_status_t UNKNOWN_OW_VERSION = 1 << 2;
    constexpr pnp_status_t OW_CORRUPTION = 1 << 3;
    constexpr pnp_status_t SERIAL_NUM_MISMATCH = 1 << 4;
    constexpr pnp_status_t SIGNATURE_NOT_ALLOWED = 1 << 5;
    constexpr pnp_status_t GENERAL_CONFIG_ERROR = 1 << 6;
    constexpr pnp_status_t CONFIGURATION_SET_MISS = 1 << 7;
    constexpr pnp_status_t CONFIGURATION_STRUCT_MISS = 1 << 8;

    /// \brief Occurs when SIGNATURE_NOT_ALLOWED is raised due to the firmware
    /// not supporing the device's default card type.
    constexpr pnp_status_t INCOMPATIBLE_CARD_TYPE = 1 << 9;
}

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

#endif

//EOF
