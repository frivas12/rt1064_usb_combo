/**
 * @file ad5683.c
 *
 * @brief Functions for ???
 *
 */

#include <asf.h>
#include "ad5683.h"
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
void dac_init(slot_nums slot, uint16_t data)
{
	dac_write(slot, DAC_WRITE_CONTROL_REG,
			AD5683_DCEN(AD5683_DC_DISABLE) | AD5683_CFG_GAIN(AD5683_AMP_GAIN_2)
			| AD5683_REF_EN(AD5683_INT_REF_ON)
			| AD5683_OP_MOD(AD5683_PWR_NORMAL)
			| AD5683_SW_RESET(AD5683_RESET_DISABLE));

	// zero the piezo
	dac_write(slot, DAC_WRITE_DAC_AND_INPUT_REG, data);
}

// MARK:  SPI Mutex Required
void dac_write(slot_nums slot, uint8_t cmd, uint16_t data)
{
	// AD5683RBRMZ-RL7 16 bit dac
	uint8_t spi_tx_data[3];

	//Command Register bits 23:20
	spi_tx_data[0] = (data >> 12) | (cmd << 4);
	spi_tx_data[1] = (data >> 4);
	spi_tx_data[2] = ((data << 4) & 0xF0);

//	uint8_t slot = info->slot;
	spi_transfer(SPI_PIEZO_DAC_NO_READ, spi_tx_data, 3);
}

