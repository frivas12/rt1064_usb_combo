//def.h

/**************************************************************************//**
 * \file defs.h
 * \author Sean Benish
 * \brief Holds commonly used data types, defines, and constants.
 *****************************************************************************/

#ifndef SRC_DEFS_H_
#define SRC_DEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "slot_types.h"

/*****************************************************************************
 * Defines
 *****************************************************************************/

#define INT_NULL                        0               ///> Integer-Type NULL

#define INVALID_DEVICE_ID               0xFFFF
#define INVALID_CONFIG_ID               0xFFFF

#define OW_CUSTOM_CONFIG(index)         ((uint16_t)(index + 0xFFF0))


/*****************************************************************************
 * Data Types
 *****************************************************************************/

typedef uint8_t CRC8_t;
typedef uint16_t struct_id_t;
typedef uint16_t device_id_t;
typedef uint16_t config_id_t;

typedef struct __attribute__((packed)) {
    slot_types slot_type;           // Must always be known.
    device_id_t device_id;      // If this is INVALID_DEVICE_ID, then we assume that save data is in
                                    // saved memory.  Otherwise, this references the LUT device type.
} device_signature_t;

typedef struct __attribute__((packed)) {
    struct_id_t struct_id;      // Identifies the data structure represented by the structure id.
    config_id_t config_id;      // Discriminator.
} config_signature_t;

/*****************************************************************************
 * Constants
 *****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* SRC_DEFS_H_ */
//EOF
