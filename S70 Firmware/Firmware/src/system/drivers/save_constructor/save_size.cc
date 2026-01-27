// save_size.cc

/**************************************************************************//**
 * \file save_constructor.cc
 * \author Sean Benish
 *****************************************************************************/

#include "save_size.hh"

#include "structure_id.hh"

#include "stepper_saves.h"
#include "encoder.h"
// #include "servo_saves.h"
#include "shutter_saves.h"

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

uint8_t save_constructor::get_subdevice_count(const slot_types slot_type)
{
    switch(slot_type)
    {
    // Stepper Types
    case ST_Stepper_type:
    case ST_HC_Stepper_type:
    case High_Current_Stepper_Card_HD:
    case ST_Invert_Stepper_BISS_type:
    case ST_Invert_Stepper_SSI_type:
    case MCM_Stepper_Internal_BISS_L6470:
    case MCM_Stepper_Internal_SSI_L6470:
    case MCM_Stepper_L6470_MicroDB15:
    case MCM_Stepper_LC_HD_DB15:
        return STEPPER_SUBDEVICES;
    
    // Servo Types
    case Servo_type:
    case Shutter_type:
        // return SERVO_SUBDEVICES;
        return 0;

    // Piezo Type
    // case Piezo_type:
    //     return PIEZO_SUBDEVICES;


    // Shutter 4 Types
    case Shutter_4_type:
    case Shutter_4_type_REV6:
        return SHUTTER_4_SUBDEVICES;


    default:
        return 0;
    }
}

uint32_t save_constructor::get_config_size(const struct_id_t id)
{
    const Struct_ID ID = static_cast<Struct_ID>(id);

    switch(ID)
    {
    case Struct_ID::ENCODER:
        return sizeof(Encoder_Save);
    case Struct_ID::LIMIT:
        return sizeof(Limits_save);
    case Struct_ID::STEPPER_CONFIG:
        return sizeof(Stepper_Config);
    case Struct_ID::STEPPER_DRIVE:
        return sizeof(Stepper_drive_params);
    case Struct_ID::STEPPER_FLAGS:
        return sizeof(Flags_save);
    case Struct_ID::STEPPER_HOME:
        return sizeof(Home_Params);
    case Struct_ID::STEPPER_JOG:
        return sizeof(Jog_Params);
    case Struct_ID::STEPPER_PID:
        return sizeof(Stepper_Pid_Save);
    case Struct_ID::STEPPER_STORE:
        return sizeof(Stepper_Store);
    case Struct_ID::SHUTTER_4_PARAMS:
        return sizeof(Shutter_4_params);
    case Struct_ID::SERVO_PARAMS:
        // return sizeof(Servo_params);
        return 0;
    case Struct_ID::SERVO_SHUTTER_PARAMS:
        // return sizeof(Shutter_params);
        return 0;
    case Struct_ID::SERVO_STORE:
        // return sizeof(Servo_Store);
        return 0;
    // case Struct_ID::PIEZO_PARAMS:
    //     return sizeof(Piezo_params);

    default:
        return 0;
    }
}

uint32_t save_constructor::get_slot_save_size(const slot_types type)
{
    switch(type)
    {
    // Stepper Types
    case ST_Stepper_type:
    case ST_HC_Stepper_type:
    case High_Current_Stepper_Card_HD:
    case ST_Invert_Stepper_BISS_type:
    case ST_Invert_Stepper_SSI_type:
    case MCM_Stepper_Internal_BISS_L6470:
    case MCM_Stepper_Internal_SSI_L6470:
    case MCM_Stepper_L6470_MicroDB15:
    case MCM_Stepper_LC_HD_DB15:
        return sizeof(Stepper_Save_Union);

    // Servo Types
    case Servo_type:
    case Shutter_type:
        // return sizeof(Servo_Save_Union);
        return 0;
    break;

    // Piezo Type
    // case Piezo_type:
    //     return sizeof(Piezo_Save_Union);
    break;

    // Shutter 4 Types
    case Shutter_4_type:
    case Shutter_4_type_REV6:
        return sizeof(Shutter_4_Save_Union);


    // Bad Types
    case NO_CARD_IN_SLOT:
    case CARD_IN_SLOT_NOT_PROGRAMMED:
    case MAX_CARD_TYPE:
    case END_CARD_TYPES:
    default:
        return 0;
    }
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

// EOF
