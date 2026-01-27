//slot_types.h

/**************************************************************************//**
 * \file slot_types.h
 * \author Sean Benish
 * \brief Holds the slot_types data type to prevent circular includes in header
 * files.
 *****************************************************************************/

#ifndef SRC_SYSTEM_SLOTS_SLOT_TYPES_H_
#define SRC_SYSTEM_SLOTS_SLOT_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Defines
 *****************************************************************************/



/*****************************************************************************
 * Data Types
 *****************************************************************************/

typedef enum __attribute__((__packed__))
{
        // 0x0000 - 0x0001 : Unknown Types
        NO_CARD_IN_SLOT = 0,
        CARD_IN_SLOT_NOT_PROGRAMMED = 1,

        /// \todo Add the finished goods part number to the comments.

        // 0x0002 - 0x7FFF : Instantiation Types
        ST_Stepper_type = 2,                                // 2
        ST_HC_Stepper_type = 3,                     // 3 42-0093 MCM6000 HC Stepper Module
        Servo_type = 4,                                             // 4 42-0098 MCM Servo Module
        Shutter_type = 5,                                   // 5 42-0098 MCM Servo Module
        OTM_Dac_type = 6,                                   // 6
        OTM_RS232_type = 7,                                 // 7
        High_Current_Stepper_Card_HD = 8,       // 8 42-0107 MCM6000 HC Stepper Module Micro DB15
        Slider_IO_type = 9,                                 // 9 42-0104 MCM6000 Slider IO Card
        Shutter_4_type = 10,                                 // 10 42-0108 MCM6000 4 Shutter Card PCB
        Piezo_Elliptec_type = 11,                    // 11
        ST_Invert_Stepper_BISS_type = 12,    // 12 42-0113 MCM Stepper ABS BISS encoder Module
        ST_Invert_Stepper_SSI_type = 13,             // 13 42-0113 MCM Stepper ABS BISS encoder Module
        Piezo_type = 14,                                             // 14 42-0123 Piezo Slot Card PCB
        MCM_Stepper_Internal_BISS_L6470 = 15,// 15 41-0128 MCM Stepper Internal Slot Card
        MCM_Stepper_Internal_SSI_L6470 = 16, // 16 41-0128 MCM Stepper Internal Slot Card
        MCM_Stepper_L6470_MicroDB15 = 17,    // 17 41-0129 MCM Stepper LC Micro DB15
        Shutter_4_type_REV6 = 18,                            // 18 42-0108 MCM6000 4 Shutter Card PCB REV 6
        MCM_Stepper_LC_HD_DB15 = 19,                         // 19 42-0132 MCM Stepper LC HD DB15
        MCM_Flipper_Shutter = 20,                            // 20 42-0161 MCM6000 Flipper Shutter Card
        MCM_Flipper_Shutter_REVA = 21,                       // 20 42-0161-A MCM6000 Flipper Shutter Card
        MAX_CARD_TYPE,


        // 0xFFFF - 0xFFFF : END_CARD_TYPES
        END_CARD_TYPES = 0xFFFF
} slot_types;

/*****************************************************************************
 * Constants
 *****************************************************************************/



/*****************************************************************************
 * Public Functions
 *****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_SLOTS_SLOT_TYPES_H_ */
//EOF
