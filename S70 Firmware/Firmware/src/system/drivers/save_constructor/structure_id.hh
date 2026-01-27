// structure_id.hh

/**************************************************************************//**
 * \file structure_id.hh
 * \author Sean Benish
 * \brief Defines all of the structures used in the config LUT.
 *****************************************************************************/

#ifndef _SRC_SYSTEM_DRIVERS_LUT_STRUCTURE_ID_HH_
#define _SRC_SYSTEM_DRIVERS_LUT_STRUCTURE_ID_HH_

#include "defs.h"

enum class Struct_ID : struct_id_t
{
    // Encoder Structures
    ENCODER                     = 0,

    // Limit Structures
    LIMIT                       = 1,

    // Stepper Structures
    STEPPER_CONFIG              = 2,
    STEPPER_DRIVE               = 3,
    STEPPER_FLAGS               = 4,
    STEPPER_HOME                = 5,
    STEPPER_JOG                 = 6,
    STEPPER_PID                 = 7,
    STEPPER_STORE               = 8,

    FLIPPER_SHUTTER_SIO         = 9,
    FLIPPER_SHUTTER_SHUTTER_COMMON = 10,
    SHUTTER_CONTROLLER_WAVEFORM = 11,
    



    // Non-enumeratred.
    // Slider IO Structures
    SLIDER_IO_PARAMS,

    // Shutter 4 Structures
    SHUTTER_4_PARAMS,

    // Servo Structures
    SERVO_PARAMS,
    SERVO_SHUTTER_PARAMS,
    SERVO_STORE,

    // Piezo Structures,
    PIEZO_PARAMS,

    INVALID                     = 0xFFFF
};

#endif

// EOF
