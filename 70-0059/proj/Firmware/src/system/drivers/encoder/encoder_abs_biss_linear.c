/**
 * @file encoder_abs_biss.c
 *
 * @brief Functions for ???
 *
 */

#include <asf.h>
#include <encoder.h>
#include <encoder_abs_biss_linear.h>
#include <cpld.h>
#include <string.h>
#include <delay.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/
bool checkCRC(uint8_t *BitString);

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
void set_abs_biss_linear_encoder_position(uint8_t slot, int32_t counts)
{

}


bool checkCRC(uint8_t *BitString)
{
	uint8_t CRC[6] = {0};
	uint8_t i;
	uint8_t DoInvert;

	for (i = 0; i < strlen((const char *)BitString); ++i)
	{
		DoInvert = ('1' == BitString[i]) ^ CRC[5];         // XOR required?

		CRC[5] = CRC[4];
		CRC[4] = CRC[3];
		CRC[3] = CRC[2];
		CRC[2] = CRC[1];
		CRC[1] = CRC[0] ^ DoInvert;
		CRC[0] = DoInvert;
	}

	/*All CRC should be equal to zero, if not there was an error*/
	for (i = 0; i < 6; ++i)
	{
		if (CRC[i] != 0)
			return 1;	/*There was an error*/
	}

	return 0;	/* No error*/
}

// MARK:  SPI Mutex Required
int32_t get_abs_biss_linear_encoder_counts(uint8_t slot, Encoder *enc)
{
#if 0
	uint32_t counts;
	uint8_t spi_tx_rx_data[8];
	uint8_t temp_vals[6];
	uint8_t temp;
	uint8_t string_packet[40];
	uint8_t error = 0;

	again:
	/*read the encoder*/
	spi_transfer(SPI_BISS_ENC_READ, spi_tx_rx_data, 6);

	/* need to shift the transfer 4 bits to the left*/
	for (uint8_t i = 0; i < 6; ++i)
	{
		temp_vals[i] = (spi_tx_rx_data[i] << 4) | ((spi_tx_rx_data[i+1] & 0xf0) >> 4);
	}

	/* encoder counts are in the first 4 bytes*/
	counts = (temp_vals[0] << 24) + (temp_vals[1] << 16) +
			(temp_vals[2] << 8) + (temp_vals[3] << 0);

	/*We need to invert the crc bits but not the 2 high bits for error and warn*/
	temp = temp_vals[4] & 0x3f;
	temp = ~temp &  0x3f;
	temp_vals[4] &= 0xc0;
	temp_vals[4] |= temp;

	/*Convert the 40 bits into a string to calculate the crc*/
	for (uint8_t byte = 0; byte < 5; ++byte)
	{
		for (uint8_t bit = 0; bit < 8; ++bit)
		{
			if (Tst_bits(temp_vals[byte], 1 << (7 -bit)))
				string_packet[(byte * 8 ) + bit] = '1';
			else
				string_packet[(byte * 8 ) + bit] = '0';
		}
	}

	/*The crc check should return 0, if not then there was an error*/
	if(checkCRC(string_packet))
	{
		error++;
		goto again;
//		counts = enc->enc_pos_raw;

	}

	/*check byte 0 for and 5 for alignment.  If they are not correct, just set the encoder
	 * value to the last value*/
	if(temp_vals[5] != 0)
	{
		error++;
		goto again;
//		counts = enc->enc_pos_raw;
	}

	if((spi_tx_rx_data[0] & 0xf0) != 0x20)
	{
		error++;
		goto again;
//		counts = enc->enc_pos_raw;
	}

	if(error > 0)
	{

	}

	/*The error bit is active low: �1� indicates that the transmitted
	 * position information has been verified by the readhead�s internal
	 * safety checking algorithm and is correct; �0� indicates that the
	 * internal check has failed and the position information should not
	 *  be trusted. The error bit is also set to �0� if the temperature
	 *   exceeds the maximum specified for the product.*/
	enc->error = Tst_bits(temp_vals[4], 0x80);		// 0 fail

	/*The warning bit is active low: �0� indicates that the encoder
	 * scale (and/or reading window) should be cleaned.*/
	enc->warn = Tst_bits(temp_vals[4], 0x40);	// 0 fail

	return counts;

#elif 0

	int32_t counts;
	uint8_t spi_tx_rx_data[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
	uint8_t temp_vals[6];
	uint8_t temp;
	uint8_t string_packet[40];
	bool error = false;

	/*read the encoder*/
	spi_transfer(SPI_BISS_ENC_READ, spi_tx_rx_data, 6);

	/* need to shift the transfer 4 bits to the left*/
	for (uint8_t i = 0; i < 6; ++i)
	{
		temp_vals[i] = (spi_tx_rx_data[i] << 4) | ((spi_tx_rx_data[i+1] & 0xf0) >> 4);
	}

	/* encoder counts are in the first 4 bytes*/
	counts = (temp_vals[0] << 24) + (temp_vals[1] << 16) +
			(temp_vals[2] << 8) + (temp_vals[3] << 0);

	/*We need to invert the crc bits but not the 2 high bits for error and warn*/
	temp = temp_vals[4] & 0x3f;
	temp = ~temp &  0x3f;
	temp_vals[4] &= 0xc0;
	temp_vals[4] |= temp;

	/*Convert the 40 bits into a string to calculate the crc*/
	for (uint8_t byte = 0; byte < 5; ++byte)
	{
		for (uint8_t bit = 0; bit < 8; ++bit)
		{
			if (Tst_bits(temp_vals[byte], 1 << (7 -bit)))
				string_packet[(byte * 8 ) + bit] = '1';
			else
				string_packet[(byte * 8 ) + bit] = '0';
		}
	}

	/*The crc check should return 0, if not then there was an error*/
	if(checkCRC(string_packet))
		error = true;

	/*check byte 0 for and 5 for alignment.  If they are not correct, just set the encoder
	 * value to the last value*/
	else if(temp_vals[5] != 0)
		error = true;

	else if((spi_tx_rx_data[0] & 0xf0) != 0x20)
		error = true;

	if(error)
	{
		counts = enc->enc_pos_raw;
		enc->error = true;
		enc->warn = true;
	}
	else
	{
		// temp for CS glitch
//		if(abs(counts - enc->enc_pos_prev_raw) > 100000)
//		{
//			counts = enc->enc_pos_raw;
//		}
//
//		enc->enc_pos_prev_raw = counts;
		// end temp


		/*The error bit is active low: �1� indicates that the transmitted
		 * position information has been verified by the readhead�s internal
		 * safety checking algorithm and is correct; �0� indicates that the
		 * internal check has failed and the position information should not
		 *  be trusted. The error bit is also set to �0� if the temperature
		 *   exceeds the maximum specified for the product.*/
		enc->error = Tst_bits(temp_vals[4], 0x80);		// 0 fail

		/*The warning bit is active low: �0� indicates that the encoder
		 * scale (and/or reading window) should be cleaned.*/
		enc->warn = Tst_bits(temp_vals[4], 0x40);	// 0 fail
	}
	return counts;
#else
	int32_t counts;
	int32_t position;
	uint8_t temp;
	// uint8_t error = 0;
	// uint8_t data[5] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	// uint8_t string_packet[40];

	/*read the encoder*/
	read_reg(C_READ_BISS_ENC, slot, &temp, (uint32_t*)&counts);

	position = counts;

	/*The error bit is active low: �1� indicates that the transmitted
	 * position information has been verified by the readhead�s internal
	 * safety checking algorithm and is correct; �0� indicates that the
	 * internal check has failed and the position information should not
	 *  be trusted. The error bit is also set to �0� if the temperature
	 *   exceeds the maximum specified for the product.*/
	enc->error = Tst_bits(temp, 0x80);		// 0 fail

	/*The warning bit is active low: �0� indicates that the encoder
	 * scale (and/or reading window) should be cleaned.*/
	enc->warn = Tst_bits(temp, 0x40);	// 0 fail



//	data[4] = temp;
//	data[3] = counts & 0xff;
//	data[2] = (counts >> 8) & 0xff;
//	data[1] = (counts >> 8) & 0xff;
//	data[0] = (counts >> 8) & 0xff;


#if 0
	for (uint8_t byte = 0; byte < 5; ++byte)
	{
		for (uint8_t bit = 0; bit < 8; ++bit)
		{
			if (Tst_bits(data[byte], 1 << (7 - bit)))
				string_packet[(byte * 8) + bit] = '1';
			else
				string_packet[(byte * 8) + bit] = '0';
		}
	}

	if (checkCRC(string_packet))
		error = true;

	if (error)
	{
		counts = enc->enc_pos_raw;
		enc->error = true;
		enc->warn = true;
	}
	else
	{

		enc->error = Tst_bits(temp, 0x80);           // 0 fail

		enc->warn = Tst_bits(temp, 0x40);           // 0 fail
	}
#endif
	return position;


#endif
}
