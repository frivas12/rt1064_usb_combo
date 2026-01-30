//slot_nums.h

/**************************************************************************//**
 * \file slot_nums.h
 * \author Sean Benish
 * \brief Holds the slot_nums data type to prevent circular includes in header
 * files.
 *****************************************************************************/

#ifndef SRC_SYSTEM_SLOTS_SLOT_NUMS_H_
#define SRC_SYSTEM_SLOTS_SLOT_NUMS_H_

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Defines
 *****************************************************************************/



/*****************************************************************************
 * Data Types
 *****************************************************************************/

typedef enum
{
	SLOT_1 = 0, SLOT_2, SLOT_3, SLOT_4, SLOT_5, SLOT_6, SLOT_7, SLOT_8, NO_SLOT = 0xff	// make this enum 8bit
}slot_nums;

/*****************************************************************************
 * Constants
 *****************************************************************************/



/*****************************************************************************
 * Public Functions
 *****************************************************************************/

#ifdef __cplusplus
}
extern "C++" slot_nums operator++(slot_nums& num);
extern "C++" slot_nums operator++(slot_nums& num, int);
#endif

#endif /* SRC_SYSTEM_SLOTS_SLOT_TYPES_H_ */
//EOF
