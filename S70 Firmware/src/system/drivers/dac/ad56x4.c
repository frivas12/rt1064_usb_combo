/**
 * @file ad56x4.c
 *
 * @brief Functions for ???
 *
 */

#include <asf.h>
#include "ad56x4.h"
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
void AD56X4UpdateChannel(uint8_t slot, uint8_t channel)
{
	uint8_t tx_packet[3];

	tx_packet[0] = AD56X4_COMMAND_UPDATE_DAC_REGISTER | channel;
	tx_packet[1] = 0;
	tx_packet[2] = 0;

	spi_transfer(SPI_AD56X4_NO_READ, tx_packet, 3);
}

/**
 * @brief  Commands the AD564X DAC whose Slave Select pin is CS to set the given
 * power mode for the specified channels. The power modes are
 *
 * AD56X4_POWERMODE_NORMAL             normal operation (power up)
 * AD56X4_POWERMODE_POWERDOWN_1K       connected to ground by 1k
 * AD56X4_POWERMODE_POWERDOWN_100K     connected to ground by 100k
 * AD56X4_POWERMODE_POWERDOWN_TRISTATE power down in tristate.
*
 * A power mode can be applied to a set of channels specified by a channel mask
 *  (bits 3 through 0 correspond to channels D through A), a boolean array (in channel
 *   D through A order), or four boolean arguments. Or, an array of power modes can
 *    be applied to each channel (in D through A order).
 *
 * @param slot
 * @param powerMode
 * @param channelMask
 */
void AD56X4PowerUpDown(uint8_t slot, uint8_t powerMode, uint8_t channelMask)
{
//	AD56X4.writeMessage(SS_pin, AD56X4_COMMAND_POWER_UPDOWN, 0,
//			word((B00110000 & powerMode) | (B00001111 & channelMask)));
}

/**
 * @brief Reset the AD56X4, which will power it up and set all outputs  to zero.
 *
 * Commands the AD56X4 DAC whose Slave Select pin is CS to reset. The DAC (output)
 * and input (buffer) registers are set to zero, and if doing a full reset, the channels are all
 * powered up, the external reference is used (internal turned off if present), and all channels
 *  set so that writing to the input if present), and all channels set so that writing to the input
 *  register does not auto update the DAC register (output).
 *
 * @param slot
 * @param fullReset
 */
// MARK:  SPI Mutex Required
void AD56X4Reset(uint8_t slot, bool fullReset)
{
	uint8_t spi_tx_data[3] = { 0 };

	spi_tx_data[0] = AD56X4_COMMAND_RESET | AD56X4_CHANNEL_ALL;
	spi_tx_data[1] = 0;
	spi_tx_data[2] = fullReset;

	spi_transfer(SPI_AD56X4_NO_READ, spi_tx_data, 3);
}

/**
 * @brief Commands the AD564X DAC whose Slave Select pin is CS which channels are to
 * have their DAC register (output) updated immediately when the input register (buffer)
 *  is set. True is for auto update and false is for not. It can either be given as a
 *   channel mask (bits 3 through 0 correspond to channels D through A), a boolean array
 *    (in channel D through A order), or four boolean arguments.
 *
 * @param slot
 * @param channelMask
 */
void AD56X4SetInputMode(uint8_t slot, uint8_t channelMask)
{
// AD56X4_COMMAND_SET_LDAC
}

// MARK:  SPI Mutex Required
void AD56X4SetChannel(uint8_t slot, uint8_t setMode, uint8_t channel, uint16_t value)
{
	uint8_t tx_packet[3];

	/*data is 12bit and starts in D11*/
	uint32_t temp_val = value << 4;

	// Don't do anything if we weren't given a valid setMode.
	if (setMode == AD56X4_SETMODE_INPUT || setMode == AD56X4_SETMODE_INPUT_DAC
			|| setMode == AD56X4_SETMODE_INPUT_DAC_ALL)
	{
		tx_packet[0] = setMode | channel;
		tx_packet[1] = temp_val>>8;
		tx_packet[2] = temp_val;

		spi_transfer(SPI_AD56X4_NO_READ, tx_packet, 3);
	}
}

// MARK:  SPI Mutex Required
void AD56X4SetOutput(uint8_t slot, uint8_t channel, uint16_t value)
{
	uint8_t tx_packet[3];

	/*data is 12bit and starts in D11*/
	uint32_t temp_val = value << 4;

	tx_packet[0] = channel | AD56X4_COMMAND_WRITE_UPDATE_CHANNEL;
	tx_packet[1] = temp_val>>8;
	tx_packet[2] = temp_val;

	spi_transfer(SPI_AD56X4_NO_READ, tx_packet, 3);
}

// MARK:  SPI Mutex Required
void InitAd5624Dac(uint8_t slot)
{
	// sets all outputs to zero
	AD56X4Reset(slot, 1);
}
