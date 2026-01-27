// user_spi.c

#include <asf.h>
#include "Debugging.h"
#include <pins.h>
#include "user_spi.h"

// TODO Temp
#include "delay.h"
#include "25lc1024.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/
/* Declare a binary Semaphore flag for the SPI Bus. To ensure only single access to SPI
 * Bus. */
SemaphoreHandle_t xSPI_Semaphore = NULL; // removed STATIC to allow other processes to use same semaphore.

/****************************************************************************
 * Private Data
 ****************************************************************************/
static bool spi0_toggle_s;
static uint8_t spi0_current_cs_s;

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void _spi_init_pins(void)
{
	/* Setup chip select pins*/
	set_pin_as_output(PIN_CS_0, 1);
	set_pin_as_output(PIN_CS_1, 1);
	set_pin_as_output(PIN_CS_2, 1);
	set_pin_as_output(PIN_CS_3, 1);
	set_pin_as_output(PIN_CS_4, 1);
	set_pin_as_output(PIN_C_CSSPIN, 1);
	set_pin_as_output(PIN_CPLD_CS, 1);
	set_pin_as_output(PIN_CS_READY, 0); /* TODO may want to take out, it's  used
	 as a trigger for CS  controller in CPLD*/
	/* Setup MISO mode MUX B*/
	((Pio *) PIOD)->PIO_ABCDSR[0] |= PIO_PD20;
	((Pio *) PIOD)->PIO_ABCDSR[1] &= ~PIO_PD20;
	/*Disables the PIO from controlling the corresponding pin (enables peripheral control of the pin).*/
	((Pio *) PIOD)->PIO_PDR = PIO_PD20;

	/* Setup MOSI mode MUX B*/
	((Pio *) PIOD)->PIO_ABCDSR[0] |= PIO_PD21;
	((Pio *) PIOD)->PIO_ABCDSR[1] &= ~PIO_PD21;
	/*Disables the PIO from controlling the corresponding pin (enables peripheral control of the pin).*/
	((Pio *) PIOD)->PIO_PDR = PIO_PD21;

	/* Setup Clock mode MUX B*/
	((Pio *) PIOD)->PIO_ABCDSR[0] |= PIO_PD22;
	((Pio *) PIOD)->PIO_ABCDSR[1] &= ~PIO_PD22;
	/*Disables the PIO from controlling the corresponding pin (enables peripheral control of the pin).*/
	((Pio *) PIOD)->PIO_PDR = PIO_PD22;
#if 0
	/* Setup chip select pin NPCS0 mode MUX D*/
	((Pio *) PIOB)->PIO_ABCDSR[0] |= PIO_PB2;
	((Pio *) PIOB)->PIO_ABCDSR[1] |= PIO_PB2;
	/*Disables the PIO from controlling the corresponding pin (enables peripheral control of the pin).*/
	((Pio *) PIOB)->PIO_PDR = PIO_PB2;

	/* Setup chip select pin NPCS1 mode MUX D*/
	((Pio *) PIOA)->PIO_ABCDSR[0] |= PIO_PA31;
	((Pio *) PIOA)->PIO_ABCDSR[1] |= PIO_PA31;
	/*Disables the PIO from controlling the corresponding pin (enables peripheral control of the pin).*/
	((Pio *) PIOA)->PIO_PDR = PIO_PA31;

	/* Setup chip select pin NPCS2 mode MUX C*/
	((Pio *) PIOD)->PIO_ABCDSR[0] &= ~PIO_PD12;
	((Pio *) PIOD)->PIO_ABCDSR[1] |= PIO_PD12;
	/*Disables the PIO from controlling the corresponding pin (enables peripheral control of the pin).*/
	((Pio *) PIOD)->PIO_PDR = PIO_PD12;

	/* Setup chip select pin NPCS3 mode MUX B*/
	((Pio *) PIOD)->PIO_ABCDSR[0] |= PIO_PD27;
	((Pio *) PIOD)->PIO_ABCDSR[1] &= ~PIO_PD27;
	/*Disables the PIO from controlling the corresponding pin (enables peripheral control of the pin).*/
	((Pio *) PIOD)->PIO_PDR = PIO_PD27;
#endif
}

/**
 * @brief C_CS is used for programming the CPLD and is connected directly to the uC.  C_CSSPIN is
 * used to communicate with the CPLD from the uC and is also connected directly to the uC.
 * CS_0, CS_1, CS_2, CS_3, and CS_4 are routed to the CPLD to mux these pins for chip select to each
 * slot or other device.
 *
 * Chip select table:
 *
 * 	CS	|	  C_CS		C_CSSPIN 	CS_4	CS_3	CS_2	CS_1	CS_0
 *  63	|      	0			1		  1		 1		 1		 1		 1		// CPLD_PROGRAM
 *  95	|      	1			0		  1		 1		 1		 1		 1		// CPLD
 *  96	|      	1			1		  0		 0		 0		 0		 0		// CS_ALL_HIGH
 *  97	|      	1			1		  0		 0		 0		 0		 1		// Slot 1 cs 1
 *  98	|      	1			1		  0		 0		 0		 1		 0		// Slot 1 cs 2
 *  99	|      	1			1		  0		 0		 0		 1		 1		// Slot 2 cs 1
 *  100	|      	1			1		  0		 0		 1		 0		 0		// Slot 2 cs 2
 *  101	|      	1			1		  0		 0		 1		 0		 1		// Slot 3 cs 1
 *  102	|      	1			1		  0		 0		 1		 1		 0		// Slot 3 cs 2
 *  103	|      	1			1		  0		 0		 1		 1		 1		// Slot 4 cs 1
 *  104	|      	1			1		  0		 1		 0		 0		 0		// Slot 4 cs 2
 *  105	|      	1			1		  0		 1		 0		 0		 1		// Slot 5 cs 1
 *  106	|      	1			1		  0		 1		 0		 1		 0		// Slot 5 cs 2
 *  107	|      	1			1		  0		 1		 0		 1		 1		// Slot 6 cs 1
 *  108	|      	1			1		  0		 1		 1		 0		 0		// Slot 6 cs 2
 *  109	|      	1			1		  0		 1		 1		 0		 1		// Slot 7 cs 1
 *  110	|      	1			1		  0		 1		 1		 1		 0		// Slot 7 cs 2
 *  125	|      	1			1		  1		 1		 1		 0		 1		// flash
 *  126	|      	1			1		  1		 1		 1		 1		 0		// MAX3421
 *
 *
 *
 * @param cs : The chip select number we want to talk to
 */
__STATIC_INLINE void chip_select(uint8_t cs)
{
	uint32_t timeout = SPI_TIMEOUT;

//	if (cs == 99)
//		return;

	/* If end of transmission or we need to pull cs high,  we need to wait for the
	 for the data in the transmit register to be empty or else the cs can go high
	 early */
	if (cs == CS_ALL_HIGH)
	{
		while (!(SPI0->SPI_SR & SPI_SR_TXEMPTY))
		{
			if (!timeout--)
			{
				break;
			}
		}
	}
#if 0
	pin_low(PIN_CS_READY);

	pin_high(PIN_CS_0);
	pin_low(PIN_CS_1);
	pin_high(PIN_CS_2);
	pin_high(PIN_CS_3);
	pin_high(PIN_CS_4);

	pin_high(PIN_CS_READY);
#elif 1
	pin_low(PIN_CS_READY);
	((cs & 1) >> 0)  ? 	pin_high(PIN_CS_0) : pin_low(PIN_CS_0);
	((cs & 2) >> 1)  ? 	pin_high(PIN_CS_1) : pin_low(PIN_CS_1);
	((cs & 4) >> 2)  ? 	pin_high(PIN_CS_2) : pin_low(PIN_CS_2);
	((cs & 8) >> 3)  ? 	pin_high(PIN_CS_3) : pin_low(PIN_CS_3);
	((cs & 16) >> 4) ? 	pin_high(PIN_CS_4) : pin_low(PIN_CS_4);
	((cs & 32) >> 5) ? 	pin_high(PIN_C_CSSPIN) : pin_low(PIN_C_CSSPIN);
	((cs & 64) >> 6) ? 	pin_high(PIN_CPLD_CS) : pin_low(PIN_CPLD_CS);
	pin_high(PIN_CS_READY);
#else
	pin_low(PIN_CS_READY);

	switch (cs)
	{
		case 95:
			pin_high(PIN_CPLD_CS);
			pin_low(PIN_C_CSSPIN);
			pin_high(PIN_CS_4);
			pin_high(PIN_CS_3);
			pin_high(PIN_CS_2);
			pin_high(PIN_CS_1);
			pin_high(PIN_CS_0);
			break;

		case 96:	// all high
			pin_high(PIN_CPLD_CS);
			pin_high(PIN_C_CSSPIN);
			pin_low(PIN_CS_4);
			pin_low(PIN_CS_3);
			pin_low(PIN_CS_2);
			pin_low(PIN_CS_1);
			pin_low(PIN_CS_0);
			break;

		case 97:
			pin_high(PIN_CPLD_CS);
			pin_high(PIN_C_CSSPIN);
			pin_low(PIN_CS_4);
			pin_low(PIN_CS_3);
			pin_low(PIN_CS_2);
			pin_low(PIN_CS_1);
			pin_high(PIN_CS_0);
			break;

		case 99:
			pin_high(PIN_CPLD_CS);
			pin_high(PIN_C_CSSPIN);
			pin_low(PIN_CS_4);
			pin_low(PIN_CS_3);
			pin_low(PIN_CS_2);
			pin_high(PIN_CS_1);
			pin_high(PIN_CS_0);
			break;

		case 103:
			pin_high(PIN_CPLD_CS);
			pin_high(PIN_C_CSSPIN);
			pin_low(PIN_CS_4);
			pin_low(PIN_CS_3);
			pin_high(PIN_CS_2);
			pin_high(PIN_CS_1);
			pin_high(PIN_CS_0);
			break;

		case 125:
			pin_high(PIN_CPLD_CS);
			pin_high(PIN_C_CSSPIN);
			pin_high(PIN_CS_4);
			pin_high(PIN_CS_3);
			pin_high(PIN_CS_2);
			pin_low(PIN_CS_1);
			pin_high(PIN_CS_0);
			break;

		case 126:
			pin_high(PIN_CPLD_CS);
			pin_high(PIN_C_CSSPIN);
			pin_high(PIN_CS_4);
			pin_high(PIN_CS_3);
			pin_high(PIN_CS_2);
			pin_high(PIN_CS_1);
			pin_low(PIN_CS_0);
			break;

		default:
			pin_high(PIN_CPLD_CS);
			pin_high(PIN_C_CSSPIN);
			pin_low(PIN_CS_4);
			pin_low(PIN_CS_3);
			pin_low(PIN_CS_2);
			pin_low(PIN_CS_1);
			pin_low(PIN_CS_0);
			break;
	}
	pin_high(PIN_CS_READY);
//	delay_us(10);
//	pin_low(PIN_CS_READY);

#endif
}

/**
 * @brief SPI protocol modes
 * 	mode	CPOL	NCPHS	Shift SPCK Edge	 	Capture SPCK Edge	SPCK Inactive Level
 * 	 0 		 0	      1		   Falling				Rising				Low
 * 	 1 		 0	      0		   Rising				Falling				Low
 * 	 2 		 1	      1		   Rising				Falling				High
 * 	 3 		 1	      0		   Falling				Rising				High
 * @param mode	SPI Mode 0-3
 */
__STATIC_INLINE void set_mode(spi_modes mode)
{
	switch (mode)
	{
	case 0:
		/*Clock polarity 	0*/
		SPI0->SPI_CSR[0] &= ~SPI_CSR_CPOL;
		/*Clock Phase    	0*/
		SPI0->SPI_CSR[0] |= SPI_CSR_NCPHA;
		break;

	case 1:
		/*Clock polarity 	0*/
		SPI0->SPI_CSR[0] &= ~SPI_CSR_CPOL;
		/*Clock Phase    	1*/
		SPI0->SPI_CSR[0] &= ~SPI_CSR_NCPHA;
		break;

	case 2:
		/*Clock polarity 	1*/
		SPI0->SPI_CSR[0] |= SPI_CSR_CPOL;
		/*Clock Phase    	0*/
		SPI0->SPI_CSR[0] |= SPI_CSR_NCPHA;
		break;

	case 3:
		/*Clock polarity 	1*/
		SPI0->SPI_CSR[0] |= SPI_CSR_CPOL;
		/*Clock Phase    	1*/
		SPI0->SPI_CSR[0] &= ~SPI_CSR_NCPHA;
		break;
	}
}

/**
 * \brief Write the transmitted data on SPI bus.
 *
 * \param tx_data The data to transmit.
 *true) {
 * \retval SPI_OK on Success.
 * \retval SPI_ERROR_TIMEOUT on Time-out.
 */
__STATIC_INLINE bool write(uint8_t tx_data)
{
	uint32_t timeout = SPI_TIMEOUT;

	while (!(SPI0->SPI_SR & SPI_SR_TDRE))
	{
		if (!timeout--)
		{
			return SPI_ERROR_TIMEOUT;
		}
	}
	SPI0->SPI_TDR = (uint16_t) tx_data;
	return SPI_OK;
}

/**
 * \brief Read the received data.
 *
 * \param data Pointer to the location where to store the received byte.
 *
 * \retval SPI_OK on Success.
 * \retval SPI_ERROR_TIMEOUT on Time-out.
 */
__STATIC_INLINE bool _read(uint8_t *rx_data)
{
	uint32_t timeout = SPI_TIMEOUT;

	while (!((SPI0->SPI_SR & (SPI_SR_RDRF | SPI_SR_TXEMPTY)) == (SPI_SR_RDRF | SPI_SR_TXEMPTY)))
	{
		if (!timeout--)
		{
			return SPI_ERROR_TIMEOUT;
		}
	}
	*rx_data = (uint8_t) ( SPI0->SPI_RDR & SPI_RDR_RD_Msk);
	return SPI_OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
#include "delay.h"
void user_spi_init(void)
{
	debug_print(" Initialize SPI as master\r\n");

	_spi_init_pins();
	chip_select(CS_ALL_HIGH);
	pin_high(PIN_CS_READY);

	spi_enable_clock(SPI0);
	spi_disable(SPI0);
	spi_reset(SPI0);
	spi_set_master_mode(SPI0);
	spi_disable_mode_fault_detect(SPI0);
	spi_disable_peripheral_select_decode(SPI0);
	spi_set_bits_per_transfer(SPI0, 0, SPI_CSR_BITS_8_BIT);
	spi_set_baudrate_div(SPI0, 0, (sysclk_get_peripheral_hz() / SPI_SPEED));
	spi_set_transfer_delay(SPI0, 0, SPI_DLYBS, SPI_DLYBCT);
	spi_enable(SPI0);

	/**
	 * Create a Mutex for sending data on the SPI port
	 * Check to see if the semaphore has not been created.
	 */
	if (xSPI_Semaphore == NULL)
	{
		/* Then create the SPI bus mutex semaphore */
		xSPI_Semaphore = xSemaphoreCreateMutex();
		if ((xSPI_Semaphore) != NULL)
		{
			xSemaphoreGive((xSPI_Semaphore)); /* make the SPI bus available */
		        NameQueueObject(xSPI_Semaphore, "SPI0 L");
		}
	}
}

/**
 * \brief Perform SPI master transfer.
 *
 * \param pbuf Pointer to buffer to transfer.
 * \param size Size of the buffer.
 */
/**
 * @brief Perform SPI master transfer
 * 
 * @warning The SPI semaphore must be owned to guarantee no thread locks.
 *
 * @param mode	: SPI protocol mode 0 - 3
 * @param toggle : Weather to toggle CS after each byte
 * @param cs	: Chip select
 * @param buf	: Buffer for holding data to send but also returns the data on reads
 * @param size : How many bytes to send
 *
 * @retval : 0 if everything went well, 1 if their was an error
 */
spi_status_t spi_transfer(spi_modes mode, bool do_read, bool toggle, uint8_t cs, uint8_t *buf,
		uint32_t size)
{
	spi_status_t rt = spi_start_transfer(mode, toggle, cs);

	if (rt == SPI_OK) {
		rt = do_read ? spi_partial_transfer_array(buf, size)
                     : spi_partial_write_array(buf, size);
	}

	if (rt == SPI_OK) {
		rt = spi_end_transfer();
	}

	return rt;
}
spi_status_t spi_start_transfer(spi_modes mode, bool toggle, uint8_t cs) {
	spi0_toggle_s = toggle;
	spi0_current_cs_s = cs;

	uint32_t timeout = SPI_TIMEOUT;
	// Wait for transmit register to be empty
	while (! (SPI0->SPI_SR & SPI_SR_TDRE))
	{
		if (!timeout--)
		{ return SPI_ERROR_TIMEOUT; }
	}

	set_mode(mode);
	chip_select(cs);

	return SPI_OK;
}

spi_status_t spi_partial_write(uint8_t value) {
    if (write(value) != SPI_OK)
    { return SPI_ERROR_TIMEOUT; }

    // Wait until transfer is complete.
    while ((spi_read_status(SPI0) & SPI_SR_RDRF) == 0);

    return SPI_OK;
}

spi_status_t spi_partial_transfer(uint8_t* inout) {
    if (write(*inout) != SPI_OK)
    { return SPI_ERROR_TIMEOUT; }

    // Wait until transfer is complete.
    while ((spi_read_status(SPI0) & SPI_SR_RDRF) == 0);

    if (_read(inout) != SPI_OK)
    { return SPI_ERROR_TIMEOUT; }

    return SPI_OK;
}

spi_status_t spi_partial_write_array(const uint8_t *buf, uint32_t size) {
	const uint8_t * const END = buf + size;

	for (; buf != END; ++buf)
	{
        spi_partial_write(*buf);
        if (spi0_toggle_s && (buf + 1 != END)) {
            chip_select(CS_ALL_HIGH);
            chip_select(spi0_current_cs_s);
        }
	}

	return SPI_OK;
}

spi_status_t spi_partial_transfer_array(uint8_t *buf, uint32_t size) {
	const uint8_t * const END = buf + size;

	for (; buf != END; ++buf)
	{
        spi_partial_transfer(buf);

		if (spi0_toggle_s && (buf + 1 != END)) {
			chip_select(CS_ALL_HIGH);
			chip_select(spi0_current_cs_s);
		}
	}

	return SPI_OK;
}


spi_status_t spi_end_transfer(void) {
	chip_select(CS_ALL_HIGH);


	return SPI_OK;
}

