//piezo_saves.h

/**************************************************************************//**
 * \file piezo_saves.h
 * \author Sean Benish
 * \brief Holds all the data structures that a piezo will save onto the LUT.
 *****************************************************************************/

#ifndef SRC_SYSTEM_CARDS_PIEZO_PIEZO_SAVES_H_
#define SRC_SYSTEM_CARDS_PIEZO_PIEZO_SAVES_H_

#include "slot_nums.h"
#include "defs.h"

#ifdef __cplusplus
#include "config_save.tcc"

extern "C"
{
#endif


/*****************************************************************************
 * Defines
 *****************************************************************************/

#define PIEZO_SUBDEVICES                1
#define PIEZO_CONFIGS_IN_SAVE           1
#define PIEZO_CONFIGS_TOTAL             1

/*****************************************************************************
 * Data Types
 *****************************************************************************/

typedef struct __attribute__((packed))
{
        slot_nums slot_encoder_on;      // this can also be the slot the stepper is on
        uint8_t gain_factor;

        // Tuning parameters
        uint32_t Kp; //!< Stores the gain for the Proportional term
        uint32_t Ki; //!< Stores the gain for the Integral term
        uint32_t Kd; //!< Stores the gain for the Derivative term
        uint32_t imax;
} Piezo_params;

typedef struct __attribute__((packed))
{
        Piezo_params params;
} Piezo_Save_Params;

typedef struct __attribute__((packed))
{
        config_signature_t params;
} Piezo_Save_Configs;

typedef union __attribute__((packed))
{
        config_signature_t array[PIEZO_CONFIGS_TOTAL];
        Piezo_Save_Configs cfg;
} Piezo_Save_Union;



#ifdef __cplusplus
}

typedef struct __attribute__((packed))
{
        ConfigSave<Piezo_Save_Params> config;
} Piezo_Save;

#endif

/*****************************************************************************
 * Constants
 *****************************************************************************/



/*****************************************************************************
 * Public Functions
 *****************************************************************************/


#endif /* SRC_SYSTEM_CARDS_PIEZO_PIEZO_SAVES_H_ */
//EOF
