// cpld.c
#include "cpld.h"

#include <asf.h>
#include "Debugging.h"
#include <pins.h>
#include <delay.h>
#include <string.h>
#include "apt.h"
#include "slot_types.h"
#include "user_spi.h"
#include "usb_slave.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
uint8_t cpld_tx_data[6];
uint8_t cpld_rx_data[6];
bool cpld_programmed;
uint8_t cpld_revision;
uint8_t cpld_revision_minor;

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static void read_cpld_revision(void);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
// MARK:  SPI Mutex Required
static void read_cpld_revision(void)
{
	uint32_t temp;
	cpld_programmed = false;
	read_reg(C_READ_CPLD_REV, REVISION_ADDRESS, NULL, &temp);

	cpld_revision = temp & 0xff;
	cpld_revision_minor = temp >> 8;

	if((cpld_revision == 0) || (cpld_revision == 255))
	{
		cpld_programmed = 0;
	}
	else
	{
		cpld_programmed = 1;
	}
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

// MARK:  SPI Mutex Required
void set_reg(uint16_t command, uint8_t address, uint8_t mid_data, uint32_t data)
{
	uint8_t spi_tx_data[8] = {0};

	spi_tx_data[0] = (command>>8) & 0x00FF;
	spi_tx_data[1] = command & 0x00ff;
	spi_tx_data[2] = address;
	spi_tx_data[3] = mid_data;
	spi_tx_data[4] = data >> 24;
	spi_tx_data[5] = data >> 16;
	spi_tx_data[6] = data >> 8;
	spi_tx_data[7] = data;

	spi_transfer(SPI_CPLD_NO_READ, spi_tx_data, 8);
}

// MARK:  SPI Mutex Required
void read_reg(uint16_t command, uint8_t address, uint8_t *mid_data, uint32_t *data)
{
	uint8_t spi_tx_data[9] = {0};

	spi_tx_data[0] = (command>>8);
	spi_tx_data[1] = command;
	spi_tx_data[2] = address;
	spi_tx_data[3] = DUMMY_HIGH;
	spi_tx_data[4] = DUMMY_HIGH;
	spi_tx_data[5] = DUMMY_HIGH;
	spi_tx_data[6] = DUMMY_HIGH;
	spi_tx_data[7] = DUMMY_HIGH;
	spi_tx_data[8] = DUMMY_HIGH;

	spi_transfer(SPI_CPLD_READ, spi_tx_data, 9);

	*mid_data = spi_tx_data[4];

	*data = spi_tx_data[8] + (spi_tx_data[7] << 8) + (spi_tx_data[6] << 16)
			+ (spi_tx_data[5] << 24);
}

void cpld_reset(void)
{
	/* Reset CPLD*/
	pin_low(PIN_CPLD_RESET);
	delay_ms(50);
	pin_high(PIN_CPLD_RESET);
	delay_ms(50);
}

/**
 * Used to read the emergency stop flag in the cpld.
 * @return	0 => no emergency stop condition.
 * 			1 => emergency stop was activated, some cards type may need to be reinitialized
 */
// MARK:  SPI Mutex Required
bool cpld_read_em_stop_flag(void)
{
	uint32_t temp;
	bool em_stop_flag = false;
	read_reg(C_READ_EM_STOP_FLAG, LED_ADDRESS, NULL, &temp);
	em_stop_flag = temp & 0x01;
	return em_stop_flag;
}

// MARK:  SPI Mutex Required
void cpld_init(void)
{
	set_pin_as_output(PIN_CPLD_PROGRAMN, 1);
	set_pin_as_output(PIN_CPLD_RESET, 1);

	// 12 MHz Clock from uC CPLD_CLK
	/* Set the PCK2 clock source to MAIN_CLK and clock divider to 0*/
	((Pmc *) PMC)->PMC_PCK[2] = 1;

	/* Enable PCK2 output to pin*/
	((Pmc *) PMC)->PMC_SCER |= PMC_SCER_PCK2;

	/* Setup the clock pin from the processor to CPLD to mode MUX C*/
	((Pio *) PIOD)->PIO_ABCDSR[0] &= ~PIO_PD31;
	((Pio *) PIOD)->PIO_ABCDSR[1] |= PIO_PD31;
	/*Disables the PIO from controlling the corresponding pin (enables peripheral control of the pin).*/
	((Pio *) PIOD)->PIO_PDR = PIO_PD31;

	cpld_reset();

	read_cpld_revision();
}

/*Temp for CPLD write/read register*/
void cpld_write_read(USB_Slave_Message *slave_message)
{
	uint8_t spi_tx_data[8];
	uint8_t response_buffer[16] = {0};

	memcpy(&spi_tx_data[0], &slave_message->extended_data_buf[0], 8);

	//block until we can send the slave_message on the SPI port
	xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
	spi_transfer(SPI_CPLD_READ, spi_tx_data, 8);
	xSemaphoreGive(xSPI_Semaphore);

	/*response with return data */
	// Header
	response_buffer[0] = MGMSG_GET_CPLD_WR;
	response_buffer[1] = MGMSG_GET_CPLD_WR >> 8;
	response_buffer[2] = 0x8;				/*Number of bytes for extended data*/
	response_buffer[3] = 0x00;
	response_buffer[4] = HOST_ID | 0x80; 	/* destination*/
	response_buffer[5] = MOTHERBOARD_ID;	/* source*/

	/*Copy the data to the response buffer*/
	memcpy(&response_buffer[6], &spi_tx_data[0], 8);

	/* block until we can send the message on the USB slave Tx port*/
	xSemaphoreTake(xUSB_Slave_TxSemaphore, portMAX_DELAY);
	udi_cdc_write_buf(response_buffer, 16);
	xSemaphoreGive(xUSB_Slave_TxSemaphore);
}

// MARK:  SPI Mutex Required
void cpld_start_slot_cards(void)
{
	xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);

	for (slot_nums slot = 0; slot < NUMBER_OF_BOARD_SLOTS; ++slot)
	{
		switch (slots[slot].card_type)
		{
		case NO_CARD_IN_SLOT:
			break;

		case ST_Stepper_type:
		case ST_HC_Stepper_type:
		case High_Current_Stepper_Card_HD:
		case MCM_Stepper_L6470_MicroDB15:
		case ST_Invert_Stepper_BISS_type:
		case MCM_Stepper_Internal_BISS_L6470:
		case ST_Invert_Stepper_SSI_type:
		case MCM_Stepper_Internal_SSI_L6470:
		case MCM_Stepper_LC_HD_DB15:
			set_reg(C_SET_ENABLE_STEPPER_CARD, slot, 0, ONE_WIRE_MODE);
			break;

		case Servo_type:
		case Shutter_type:
			set_reg(C_SET_ENABLE_SERVO_CARD, slot, 0, ONE_WIRE_MODE);
			break;

		case OTM_Dac_type:
			set_reg(C_SET_ENABLE_OTM_DAC_CARD, slot, 0, ONE_WIRE_MODE);
			break;

		case OTM_RS232_type:
			set_reg(C_SET_ENABLE_RS232_CARD, slot, 0, ONE_WIRE_MODE);
			break;

		case Slider_IO_type:
			set_reg(C_SET_ENABLE_IO_CARD, slot, 0, ONE_WIRE_MODE);
			break;

		case Shutter_4_type:
		case Shutter_4_type_REV6:
        // case MCM_Flipper_Shutter:   Moved to the initialization of the thread.
			set_reg(C_SET_ENABLE_SHUTTER_CARD, slot, 0, ONE_WIRE_MODE);
			break;

		case Piezo_Elliptec_type:
			set_reg(C_SET_ENABLE_PEIZO_ELLIPTEC_CARD, slot, 0, ONE_WIRE_MODE);
			break;

		case Piezo_type:
			set_reg(C_SET_ENABLE_PEIZO_CARD, slot, 0, ONE_WIRE_MODE);
			break;

		default:
			break;
		}
	}
	xSemaphoreGive(xSPI_Semaphore);
}
