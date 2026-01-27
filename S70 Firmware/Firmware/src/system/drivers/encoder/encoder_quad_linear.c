/**
 * @file encoder_quad_linear.c
 *
 * @brief Functions for ???
 *
 */

#include <asf.h>
#include <encoder.h>
#include <encoder_quad_linear.h>
#include <cpld.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/
// MARK:  SPI Mutex Required
void set_quad_linear_encoder_position(uint8_t slot, int32_t counts)
{
	set_reg(C_SET_QUAD_COUNTS, slot, 0, (uint32_t) counts);
}

// MARK:  SPI Mutex Required
int32_t get_quad_linear_encoder_counts(uint8_t slot)
{
	uint32_t counts;

	read_reg(C_READ_QUAD_COUNTS, slot, NULL, &counts);
	return counts;
}
