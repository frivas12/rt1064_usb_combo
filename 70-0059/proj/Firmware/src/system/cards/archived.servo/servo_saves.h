//servo_saves.h

/**************************************************************************//**
 * \file servo_saves.h
 * \author Sean Benish
 * \brief Holds all the data structures that a servos will save onto the LUT.
 *****************************************************************************/

#ifndef SRC_SYSTEM_CARDS_SERVO_SERVO_SAVES_H_
#define SRC_SYSTEM_CARDS_SERVO_SERVO_SAVES_H_

#include "defs.h"

#ifdef __cplusplus
#include "config_save.tcc"

extern "C"
{
#endif

/*****************************************************************************
 * Defines
 *****************************************************************************/

#define SERVO_SUBDEVICES                1
#define SERVO_CONFIGS_IN_SAVE           2
#define SERVO_CONFIGS_TOTAL             3

/*****************************************************************************
 * Data Types
 *****************************************************************************/

typedef enum
{
        SERVO_NO_POSITION = 0,
        SERVO_POSITION_1,
        SERVO_POSITION_2
} servo_positions;

typedef enum
{
        SHUTTER_OPENED = 0,
        SHUTTER_CLOSED

} shutter_positions;

typedef enum
{
        SHUTTER_DISABLED = 0,
        SHUTTER_PULSED,
        SHUTTER_PULSE_HOLD
} shutter_mode;

typedef enum
{
        DISABLED = 0,
        ENABLED,
        REVERSED,
        TOGGLE
} shutter_trig_mode;





typedef struct  __attribute__((packed))
{
        uint32_t lTransitTime;

} Servo_params;

typedef struct  __attribute__((packed))
{
        shutter_positions initial_state;
        shutter_mode mode;      /*Some shutters are pulsed to toggle position others are pulsed with a voltage
                                 then held at a lower voltage*/
        shutter_trig_mode external_trigger_mode;
        uint8_t on_time;                        /*shutter cycle time in 10's of ms (only using 1 byte*/
        uint32_t duty_cycle_pulse;      /* used for shutter to control the voltage on the pulse*/
        uint32_t duty_cycle_hold;       /* used for shutter to control the voltage on the pulse*/

} Shutter_params;

typedef struct __attribute__((packed))
{
        servo_positions position;
        uint32_t pwm;   /*this sets the speed of the motor by changing the PWM high signal.
                                         0 = off & 1024 is 12V */
} Servo_Store;

typedef struct __attribute__((packed))
{
        config_signature_t servo_params;
        config_signature_t shutter_params;
        config_signature_t servo_store;
} Servo_Save_Configs;

typedef union __attribute__((packed))
{
        config_signature_t array[SERVO_CONFIGS_TOTAL];
        Servo_Save_Configs cfg;
} Servo_Save_Union;


typedef struct  __attribute__((packed))
{
        Servo_params params;
        Shutter_params shutter_params;
} Servo_Save_Params;



#ifdef __cplusplus
}

typedef struct
{
        ConfigSave<Servo_Save_Params> config;
        Servo_Store store;
} Servo_Save;

#endif

/*****************************************************************************
 * Constants
 *****************************************************************************/



/*****************************************************************************
 * Public Functions
 *****************************************************************************/


#endif /* SRC_SYSTEM_CARDS_SERVO_SERVO_SAVES_H_ */
//EOF
