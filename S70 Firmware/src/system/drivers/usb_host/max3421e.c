/**
 * @file max3421e.c
 *
 * @brief Lower level function for the MAX3421E USB IC.  Here we are using it in
 * host mode.
 *
 *
 */

#include <asf.h>
#include <pins.h>
#include "max3421e.h"
#include "user_spi.h"
#include "delay.h"
#include "Debugging.h"
#include "string.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
usb_host_bus_states vbusState;

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static void MaxIntHandler(void);
static void MAX3421E_busprobe(void);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/
/**
 * @brief The MAX3421e interrupt pin is polled instead of tying to an interrupt.
 *
 */
// MARK:  SPI Mutex Required
static void MaxIntHandler(void)
{
	uint8_t HIRQ;
	uint8_t HIRQ_sendback = 0x00;
	HIRQ = max3421e_read( rHIRQ);                  //determine interrupt source
	if (HIRQ & bmCONDETIRQ)
	{
		// device must have been connected or disconnected
		MAX3421E_busprobe();
		HIRQ_sendback |= bmCONDETIRQ;
	}
	/* End HIRQ interrupts handling, clear serviced IRQs    */
	max3421e_write( rHIRQ, HIRQ_sendback);
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/**
 * @brief Reset MAX3421E using chip reset bit. SPI configuration is not affected.
 *
 * @return 0 if ok, 1 timeout
 */

// MARK:  SPI Mutex Required
static bool reset(void)
{
	uint8_t tmp = 0;
	max3421e_write( rUSBCTL, bmCHIPRES); /* Chip reset. This stops the oscillator*/
	max3421e_write( rUSBCTL, 0x00); /* Remove the reset*/
	while (!(max3421e_read( rUSBIRQ) & bmOSCOKIRQ))
	{ /*wait until the PLL stabilizes*/
		tmp++; /* timeout after 256 attempts*/
		delay_ms(1);
		if (tmp == 0)
			return 1;
	}
	return 0;
}

/**
 * Probe bus to determine device presence and speed and switch host to this speed.
 *
 *	States
 * 	vbusState = FSHOST, full speed device connected
 * 	vbusState = LSHOST, low speed device connected
 * 	vbusState = SE1, illegal state
 * 	vbusState = SE0, USB device disconnected
 */
// MARK:  SPI Mutex Required
static void MAX3421E_busprobe(void)
{
	uint8_t bus_sample;
	bus_sample = max3421e_read( rHRSL);            		//Get J,K status
	bus_sample &= ( bmJSTATUS | bmKSTATUS);            		//zero the rest of the byte
	switch (bus_sample)
	{                          //start full-speed or low-speed host
		case ( bmJSTATUS):
			if ((max3421e_read( rMODE) & bmLOWSPEED) == 0)
			{
				max3421e_write( rMODE, MODE_FS_HOST);       //start full-speed host
				vbusState = FSHOST;
				usb_host_print("Full-Speed Device Detected\n\n");
			}
			else
			{
				max3421e_write( rMODE, MODE_LS_HOST);        //start low-speed host
				vbusState = LSHOST;
				usb_host_print("Low-Speed Device Detected\n\n");
			}
		break;

		case ( bmKSTATUS):
			if ((max3421e_read( rMODE) & bmLOWSPEED) == 0)
			{
				max3421e_write( rMODE, MODE_LS_HOST);       //start low-speed host
				vbusState = LSHOST;
				usb_host_print("Low-Speed Device Detected\n\n");
			}
			else
			{
				max3421e_write( rMODE, MODE_FS_HOST);       //start full-speed host
				vbusState = FSHOST;
				usb_host_print("Full-Speed Device Detected\n\n");
			}
		break;

		case ( bmSE1):              //illegal state
			vbusState = SE1;
		error_print("\nERROR: USB host device illegal state.\n");
		break;

		case ( bmSE0):              //disconnected state
			max3421e_write( rMODE, bmDPPULLDN | bmDMPULLDN | bmHOST | bmSEPIRQ);
			vbusState = SE0;
		break;
	}              //end switch( bus_sample )

}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/**
 * @brief Setup all io pins for the MAX3421e and configure as host.  Top level
 * functions must call xSemaphoreTake for the SPI mutex xSPI_Semaphore to avoid
 * data corruption.
 *
 * @return : 0 OK, 1 fail
 */
// MARK:  SPI Mutex Required
bool max3421e_init(void)
{
	/* Set the PCK0 clock source to MAIN_CLK and clock divider to 0*/
	((Pmc *) PMC)->PMC_PCK[0] = 1;

	/* Enable PCK0 output to pin*/
	((Pmc *) PMC)->PMC_SCER |= PMC_SCER_PCK0;

	/* Setup the clock pin from the processor to CPLD to mode MUX D*/
	((Pio *) PIOB)->PIO_ABCDSR[0] |= PIO_PB12;
	((Pio *) PIOB)->PIO_ABCDSR[1] |= PIO_PB12;
	/*Disables the PIO from controlling the corresponding pin (enables peripheral control of the pin).*/
	((Pio *) PIOB)->PIO_PDR = PIO_PB12;

	/* Set the interrupt pin on the MAX3421e as an input.  We pool this pin rather than
	 * connect it to an interrupt */
	set_pin_as_input(PIN_MAX_INT);

	/*  GPX is not used but setup as input*/
	set_pin_as_input(PIN_MAX_GPX_INT);

	/* Set reset pin as output, active low*/
	set_pin_as_output(PIN_MAX_RES, 0);

	/*Set PIN_MAX_PWR_EN pin as output.  This is the power enable pin to usb devices. High : power on*/
	set_pin_as_output(PIN_MAX_PWR_EN, 1);

	/* Set PIN_MAX_PWR_FAULT as an input.  This pin signals a fault on the USB power*/
	set_pin_as_input(PIN_MAX_PWR_FAULT);

	/* Enable device */
	pin_high(PIN_MAX_RES);
	delay_ms(5);

	/* Configure full-duplex SPI, GPX, and interrupt level */
	max3421e_write(rPINCTL, (bmFDUPSPI | bmINTLEVEL | bmGPXB));	// 0x1A

	/* Soft reset*/
	if (reset())
	{
		error_print("ERROR: Reseting the MAX3421e USB host .\r\n");
		return 1;
	}

	/* configure host operation */
	max3421e_write(rMODE, bmDPPULLDN | bmDMPULLDN | bmHOST | bmSEPIRQ); /* set pull-downs, Host, Separate GPIN IRQ on GPX*/

	max3421e_write(rHIEN, bmCONDETIE); /* connection detection*/
	max3421e_write(rHCTL, bmSAMPLEBUS); /*  sample USB bus*/

	uint8_t t = max3421e_read(rHCTL);
	while (!(t & bmSAMPLEBUS)) /* wait for sample operation to finish*/
	{
		t = max3421e_read(rHCTL);
		// TODO may want to put a limit to how long this waits
	}
	MAX3421E_busprobe(); /* check if anything is connected*/

	max3421e_write(rHIRQ, bmCONDETIRQ); /* clear connection detect interrupt*/
	max3421e_write(rCPUCTL, 0x01); /* enable interrupt pin*/
	return 0;
}

/**
 * @brief Write a byte to a register in the MAX3421e.  Top level functions must call
 * xSemaphoreTake for the SPI mutex xSPI_Semaphore to avoid data corruption.
 *
 * @param reg : MAX3421e register to write to
 * @param val : the byte value to write to the register
 */
// MARK:  SPI Mutex Required
void max3421e_write(uint8_t reg, uint8_t val)
{
	uint8_t spi_tx_data[2];

	spi_tx_data[0] = (reg + 2); /* + 2 means write command*/
	spi_tx_data[1] = val;
	spi_transfer(SPI_MAX3421_NO_READ, spi_tx_data, 2);
}

/**
 * @brief Write multiple bytes to the MAX3421E
 * @param reg : MAX3421E register to write to.
 * @param nbytes : Number of bytes to write.
 * @param data : Pointer to the data we are writing.
 */
// MARK:  SPI Mutex Required
void max3421e_write_bytes(uint8_t reg, uint8_t nbytes, uint8_t* data)
{
	uint8_t temp_data[nbytes+1];

	temp_data[0] = reg + 2;	/* first byte is the command, +2 to indicate it is a write*/
	memcpy(temp_data+1, data, sizeof(temp_data));
	spi_transfer(SPI_MAX3421_NO_READ, temp_data, nbytes+1);
}

/**
 * @brief Read a byte from a register in the MAX3421e.  Top level functions must call
 * xSemaphoreTake for the SPI mutex xSPI_Semaphore to avoid data corruption.
 *
 * @param reg : MAX3421e register to read to
 * @returns : Returns the byte read from the register set in the parameter
 */
// MARK:  SPI Mutex Required
uint8_t max3421e_read(uint8_t reg)
{
	uint8_t spi_tx_rx_data[2];

	spi_tx_rx_data[0] = reg;
	spi_tx_rx_data[1] = 0xFF;
	spi_transfer(SPI_MAX3421_READ, spi_tx_rx_data, 2);
	return spi_tx_rx_data[1];
}

// MARK:  SPI Mutex Required
uint8_t * max3421e_readMultiple(uint8_t reg, uint8_t count, uint8_t * values)
{
	uint8_t spi_tx_rx_data[count + 1];
	spi_tx_rx_data[0] = reg;
	spi_tx_rx_data[1] = 0xFF;
	spi_transfer(SPI_MAX3421_READ, spi_tx_rx_data, count);
	memcpy(values, spi_tx_rx_data + 1, count);
	return values;
}

/**
 * @brief Check the MAX3421e interrupt pin.
 *
 * @return vbusState
 *	States
 * 	vbusState = FSHOST, full speed device connected
 * 	vbusState = LSHOST, low speed device connected
 * 	vbusState = SE1, illegal state
 * 	vbusState = SE0, USB device disconnected
 * 	vbusState = NO_CHANGE, no interrupt detect so return no change
 */
usb_host_bus_states MAX3421E_Task(void)
{
	static bool first_run = true;
	usb_host_bus_states ret_val = NO_CHANGE;

	if (!get_pin_level(PIN_MAX_INT) || first_run)
	{
		xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
		MaxIntHandler();
		xSemaphoreGive(xSPI_Semaphore);
		ret_val = vbusState;
		first_run = false;
	}

#if 0	/* GPX not used*/
	if (MAX3421E_GPX == 1)
	{
		MaxGpxHandler();
	}
#endif
	return ret_val;
}
