//shutter_save.h

/**************************************************************************//**
 * \file shutter_saves.h
 * \author Sean Benish
 * \brief Holds all the data structures that a shutters will save onto the LUT.
 *****************************************************************************/

#ifndef SRC_SYSTEM_CARDS_SHUTTER_SHUTTER_SAVES_H_
#define SRC_SYSTEM_CARDS_SHUTTER_SHUTTER_SAVES_H_

#include "defs.h"

#ifdef __cplusplus
#include "config_save.tcc"

extern "C"
{
#endif

/*****************************************************************************
 * Defines
 *****************************************************************************/

#define SHUTTER_MAX_CHAN	        4

#define SHUTTER_4_SUBDEVICES            SHUTTER_MAX_CHAN
#define SHUTTER_4_CONFIGS_IN_SAVE       1
#define SHUTTER_4_CONFIGS_TOTAL         SHUTTER_4_CONFIGS_IN_SAVE

/*****************************************************************************
 * Data Types
 *****************************************************************************/

typedef enum
{
        SHUTTER_4_OPENED = 0,
        SHUTTER_4_CLOSED

} shutter_4_positions;

typedef enum
{
        SHUTTER_4_DISABLED = 0,
        SHUTTER_4_PULSED,
        SHUTTER_4_PULSE_HOLD,
        SHUTTER_4_PULSE_NO_REVERSE
} shutter_4_mode;

typedef enum
{
        SHUTTER_4_TRIG_DISABLED = 0,
        SHUTTER_4_TRIG_ENABLED,
        SHUTTER_4_TRIG_REVERSED,
        SHUTTER_4_TRIG_TOGGLE
} shutter_4_trig_mode;





typedef struct  __attribute__((packed))
{
        shutter_4_positions initial_state;
        shutter_4_mode mode;    /*Some shutters are pulsed to toggle position others are pulsed with a voltage
                                 then held at a lower voltage*/
        shutter_4_trig_mode external_trigger_mode;
        uint8_t on_time;                        /*shutter cycle time in 10's of ms (only using 1 byte*/
        uint32_t duty_cycle_pulse;      /* used for shutter to control the voltage on the pulse*/
        uint32_t duty_cycle_hold;       /* used for shutter to control the voltage on the hold*/

} Shutter_4_params;

typedef struct __attribute__((packed))
{
        config_signature_t parameters;
} Shutter_4_Save_Configs;

typedef union __attribute__((packed))
{
        config_signature_t array[SHUTTER_4_SUBDEVICES*SHUTTER_4_CONFIGS_TOTAL];
        Shutter_4_Save_Configs cfg[SHUTTER_4_SUBDEVICES];
} Shutter_4_Save_Union;

typedef struct  __attribute__((packed))
{
        Shutter_4_params params; /*Each shutter uses a page in EEPROM*/
} Shutter_Save_Params;


#ifdef __cplusplus
}

typedef struct
{
        ConfigSave<Shutter_Save_Params> config;
} Shutter_Save;

#endif

/*****************************************************************************
 * Constants
 *****************************************************************************/



/*****************************************************************************
 * Public Functions
 *****************************************************************************/


#endif /* SRC_SYSTEM_CARDS_SHUTTER_SHUTTER_SAVES_H_ */
//EOF
