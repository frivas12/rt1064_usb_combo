/**
 * @file cpld_program.c
 *
 * @brief Functions for CPLD programming jed file.
 *
 */

#include <asf.h>
#include "FreeRTOSConfig.h"
#include "apt.h"
#include <delay.h>
#include <pins.h>
#include "Debugging.h"
#include "cpld.h"
#include "cpld_program.h"
#include "string.h"
#include "board.h"
#include "user_spi.h"

bool load_lattice_jed;

/****************************************************************************
 * Private Data
 ****************************************************************************/
typedef struct
{
	uint8_t jed_file_portion;
	uint16_t line_count;
	uint16_t line_position;
	uint8_t rcv_data[16];
	uint8_t comp_data[16];	// used to compare data
} CPLD;
CPLD *pcpld;

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static void toggle_program_pin(void);
//static bool check_device_id(void);
static void check_busy(void);
static void transmit(uint8_t command, uint8_t opcode_1, uint8_t opcode_2,
		uint8_t opcode_3);
static void set_line_address(uint8_t mem, uint16_t line);
static void _do_write_page(const uint8_t *buf);
static void read_page(uint8_t *buf);
bool verify_data(const uint8_t * buffer);
static uint8_t read_status_reg(void);
static void program_flash_exit(bool error);
static void respond_to_host(uint8_t status);
static size_t configuration_start(const uint8_t * buffer, size_t length);
static size_t write_page(const uint8_t * buffer, size_t length);
static size_t write_feature_row(const uint8_t * buffer, size_t length);
static size_t write_fea_bits(const uint8_t * buffer, size_t length);
static void flash_program_done(void);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void toggle_program_pin(void)
{
	pin_low(PIN_CPLD_PROGRAMN);
	delay_us(10);
	pin_high(PIN_CPLD_PROGRAMN);
	delay_us(50);
}

#if 0
static bool check_device_id(void)
{
	uint8_t i = 0;
	uint8_t opcode[4] =
	{ IDCODE_PUB };
	uint8_t spi_tx_data[8];

	spi_tx_data[i++] = opcode[0];
	spi_tx_data[i++] = opcode[1];
	spi_tx_data[i++] = opcode[2];
	spi_tx_data[i++] = opcode[3];
	spi_tx_data[i++] = DUMMY_HIGH;
	spi_tx_data[i++] = DUMMY_HIGH;
	spi_tx_data[i++] = DUMMY_HIGH;
	spi_tx_data[i++] = DUMMY_HIGH;

	spi_transfer(CPLD_PROGRAM_READ, spi_tx_data, i);

	uint32_t device_id = spi_tx_data[4] + (spi_tx_data[5] << 8)
			+ (spi_tx_data[6] << 16) + (spi_tx_data[7] << 24);

	return 0;
}
#endif

// Mark:  SPI(Programming) Mutex Required
static void check_busy(void)
{
	uint8_t i = 0;
	uint8_t opcode[4] =
	{ LSC_CHECK_BUSY };
	uint8_t spi_tx_data[5];

	do
	{
		i = 0;
		spi_tx_data[i++] = opcode[0];
		spi_tx_data[i++] = opcode[1];
		spi_tx_data[i++] = opcode[2];
		spi_tx_data[i++] = opcode[3];
		spi_tx_data[i++] = DUMMY_HIGH;

		spi_transfer(CPLD_PROGRAM_READ, spi_tx_data, i);
		delay_us(10);
	} while (spi_tx_data[4] != 0);  // TODO make while loop expire
}

// Mark:  SPI(Programming) Mutex Required
static uint8_t read_status_reg(void)
{
	uint8_t i = 0;
	uint8_t opcode[4] =
	{ LSC_READ_STATUS };
	uint8_t spi_tx_data[8];

	/* Bit 		1 			0		*/
	/* 12 	   Busy 	  Ready		*/
	/* 13 	  Fail 			OK		*/

	spi_tx_data[i++] = opcode[0];
	spi_tx_data[i++] = opcode[1];
	spi_tx_data[i++] = opcode[2];
	spi_tx_data[i++] = opcode[3];
	spi_tx_data[i++] = DUMMY_HIGH;
	spi_tx_data[i++] = DUMMY_HIGH;
	spi_tx_data[i++] = DUMMY_HIGH;
	spi_tx_data[i++] = DUMMY_HIGH;

	spi_transfer(CPLD_PROGRAM_READ, spi_tx_data, i);

	return spi_tx_data[5];
}

// Mark:  SPI(Programming) Mutex Required
static void transmit(uint8_t command, uint8_t opcode_1, uint8_t opcode_2,
		uint8_t opcode_3)
{
	uint8_t i = 0;
	uint8_t spi_tx_data[4];

	spi_tx_data[i++] = command;
	spi_tx_data[i++] = opcode_1;
	spi_tx_data[i++] = opcode_2;
	spi_tx_data[i++] = opcode_3;

	spi_transfer(CPLD_PROGRAM_NO_READ, spi_tx_data, i);
}

// Mark:  SPI(Programming) Mutex Required
static void set_line_address(uint8_t mem, uint16_t line)
{
	uint8_t opcode[8] =
	{ LSC_WRITE_ADDRESS };
	uint8_t spi_tx_data[8];

	spi_tx_data[0] = opcode[0];
	spi_tx_data[1] = opcode[1];
	spi_tx_data[2] = opcode[2];
	spi_tx_data[3] = opcode[3];
	spi_tx_data[4] = mem;
	spi_tx_data[5] = 0;
	spi_tx_data[6] = line >> 8;
	spi_tx_data[7] = line;

	spi_transfer(CPLD_PROGRAM_NO_READ, spi_tx_data, 8);
}

// Mark:  SPI(Programming) Mutex Required
static void _do_write_page(const uint8_t *buf)
{
	uint8_t opcode[8] =
	{ LSC_PROG_INCR_NV };
	uint8_t spi_tx_data[4 + BYTES_PER_LINE];

	spi_tx_data[0] = opcode[0];
	spi_tx_data[1] = opcode[1];
	spi_tx_data[2] = opcode[2];
	spi_tx_data[3] = opcode[3];

	/* copy the data for the USB buffer to our SPI transmit buffer*/
	memcpy(&spi_tx_data[4], &buf[0], BYTES_PER_LINE);

	spi_transfer(CPLD_PROGRAM_NO_READ, spi_tx_data, 4 + BYTES_PER_LINE);
}

// Mark:  SPI(Programming) Mutex Required
static void read_page(uint8_t *buf)
{
	uint8_t opcode[8] =
	{ LSC_READ_INCR_NV };
	uint8_t spi_tx_data[4 + BYTES_PER_LINE];

	spi_tx_data[0] = opcode[0];
	spi_tx_data[1] = opcode[1];
	spi_tx_data[2] = opcode[2];
	spi_tx_data[3] = opcode[3];

	/*Fill in the dummy variable for SPI*/
	for (int i = 4; i < 4 + BYTES_PER_LINE; ++i)
	{
		spi_tx_data[i] = DUMMY_HIGH;
	}

	spi_transfer(CPLD_PROGRAM_READ, spi_tx_data, 4 + BYTES_PER_LINE);

	/* copy the data to the passed buffer*/
	memcpy(buf, &spi_tx_data[4], BYTES_PER_LINE);
}

bool verify_data(const uint8_t* buffer)
{
	uint8_t read_buf[BYTES_PER_LINE];

	read_page(&read_buf[0]);

	/*Compare the data read with the data we just wrote to this page*/

	/* if verify fails return 1*/
	for (int i = 0; i < BYTES_PER_LINE; i++)
	{
		if (buffer[i] != read_buf[i])
			return 1;
	}
	return 0;
}

static void program_flash_exit(bool error)
{
	/*Set the SPI speed back to normal if this was changed because it was the first time
	 * programming the CPLD*/
//	if(cpld_programmed == false)
//	{
	spi_set_baudrate_div(SPI0, 0, (sysclk_get_peripheral_hz() / SPI_SPEED));
//	}

	if (error)
	{
		load_lattice_jed = false;

		respond_to_host(CPLD_PROG_ERROR);
		debug_print("ERROR: CPLD Programming failed.");

		/*Free memory*/
		if (pcpld != NULL)
			free(pcpld);

		resume_all_task();              ///> \bug Does this ensure that other tasks will get shut down?
		cpld_programmed = false;
	}
	else
	{
		respond_to_host(CPLD_PROG_OK);
		restart_board();
		cpld_programmed = true;
	}
}

static void respond_to_host(uint8_t status)
{
	uint8_t response_buffer[6] =
	{ 0 };

	// Response Header
	response_buffer[0] = MGMSG_CPLD_UPDATE;
	response_buffer[1] = MGMSG_CPLD_UPDATE >> 8;
	response_buffer[2] = status;  //;
	response_buffer[3] = 0x00;
	response_buffer[4] = HOST_ID; 	// destination
	response_buffer[5] = MOTHERBOARD_ID;	// source

	udi_cdc_write_buf(response_buffer, 6);
}

// Mark:  SPI(Programming) Mutex Required
static size_t configuration_start(const uint8_t * buffer, size_t length)
{
    if (length < 2) {
        return 0;
    }

    uint8_t opcode[8] = {LSC_INIT_ADDRESS};
	uint8_t spi_tx_data[4];

	/* get the number of lines for the configuration flash
	 * Because their is no header the line count is stored in the first
	 * 2 bytes of the structure which happens to be ucMessageID*/
	pcpld->line_count = buffer[0]
			+ (buffer[1] << 8);

	// Transmit Reset Configuration Address
    memcpy(spi_tx_data, opcode, 4);

	spi_transfer(CPLD_PROGRAM_NO_READ, spi_tx_data, 4);

	// setup next portion
	pcpld->jed_file_portion = CONFIGURATION_FLASH_PROGRAM;
	pcpld->line_position = 0;

	respond_to_host(CPLD_PROG_OK);

    return 2;
}

static size_t write_page(const uint8_t* buffer, size_t length)
{
	// grab the line of data
	// make sure we have all the data
	if (length < 16)
	{
        return 0;
	}

	set_line_address(CONFIGURATION_FLASH_MEM, pcpld->line_position);
	delay_us(10);

	// write it to the Lattice flash
	// Transmit PROGRAM command and 128 Bits of Data (line 16 bytes)
	_do_write_page(buffer);
	delay_us(200);

	set_line_address(CONFIGURATION_FLASH_MEM, pcpld->line_position);
	delay_us(10);

	if (verify_data(buffer))
	{
		program_flash_exit(CPLD_PROG_ERROR);
	}
	else
	{
		pcpld->line_position++;

		if (pcpld->line_position == pcpld->line_count)
		{
			pcpld->jed_file_portion = WRITE_FEATURE_ROW;
		}
		respond_to_host(CPLD_PROG_OK);
	}

    return 16;
}

static size_t write_feature_row(const uint8_t * buffer, size_t length)
{
    if (length < 8) { return 0; }
	/* Changing the Feature Row can prevent the MachXO2 from configuring*/

#if 1 /* Don't program Feature row */

#else

	uint8_t spi_tx_data[4 + 8];

	/* grab the line of data*/

	/* Program the Feature Row bits*/
	spi_tx_data[0] = LSC_PROG_FEATURE;
	spi_tx_data[1] = 0;
	spi_tx_data[2] = 0;
	spi_tx_data[3] = 0;

	/* copy the data for the USB buffer to our SPI transmit buffer*/
	memcpy(&spi_tx_data[4], &buffer[0], 8);

	/* Don't need to write feature row and it is dangerous to write, if something
 	 goes wrong with the SPI enable bit we would not be able to reprogram the CPLD
 	 without JTAG*/
	spi_transfer(CPLD_PROGRAM_NO_READ, spi_tx_data, 4 + 8);

	/* Read the Feature Row bits*/
	spi_tx_data[0] = LSC_READ_FEATURE;
	spi_tx_data[1] = 0;
	spi_tx_data[2] = 0;
	spi_tx_data[3] = 0;

	for (int i = 4; i < 4 + 8; ++i)
	{
		spi_tx_data[i] = DUMMY_HIGH;
	}

	spi_transfer(CPLD_PROGRAM_READ, spi_tx_data, 4 + 8);

	/*Compare the data to verify*/
	for (int i = 0; i < 8; ++i)
	{
		if(buffer[i] != spi_tx_data[4 + i])
		{
			/* ERROR*/
			program_flash_exit(CPLD_PROG_ERROR);
			return;
		}
	}
#endif

	pcpld->jed_file_portion = WRITE_FEABITS;
	respond_to_host(CPLD_PROG_OK);

    return 8;
}

// Mark:  SPI(Programming) Mutex Required
static size_t write_fea_bits(const uint8_t * buffer, size_t length)
{
    if (length < 2) { return 0; }
#if 1 /* Don't program Feature row */
    uint8_t opcode[8] = {LSC_READ_FEABITS};
	uint8_t spi_tx_data[4 + 2];

	/* Read the FEABITS*/
    memcpy(spi_tx_data, opcode, 4);
	spi_tx_data[4] = DUMMY_HIGH;
	spi_tx_data[5] = DUMMY_HIGH;

	spi_transfer(CPLD_PROGRAM_READ, spi_tx_data, 6);

	respond_to_host(CPLD_PROG_OK);
	flash_program_done();

#else
	uint8_t tries = 0;
	uint8_t spi_tx_data[4 + 2];

	/* read the fea bits and compare against the new one, if they are the same don't
	 * 	program it*/

	/* Read the FEABITS*/
	spi_tx_data[0] = LSC_READ_FEABITS;
	spi_tx_data[1] = 0;
	spi_tx_data[2] = 0;
	spi_tx_data[3] = 0;
	spi_tx_data[4] = DUMMY_HIGH;
	spi_tx_data[5] = DUMMY_HIGH;

	spi_transfer(CPLD_PROGRAM_READ, spi_tx_data, 6);

	/*Compare the data*/
	verify:
	for (int i = 0; i < 2; ++i)
	{
		if(buffer[i] == spi_tx_data[4 + i])
		{
			/* they are the same, we don't need to write them*/
			pcpld->jed_file_portion = CONFIGURATION_FLASH_START;
			respond_to_host(CPLD_PROG_OK);
			flash_program_done();
			return;
		}
	}

	/*The data is different so we need to write the FEA bits */

	/* Program the FEABITS*/
	spi_tx_data[0] = LSC_PROG_FEABITS;
	spi_tx_data[1] = 0;
	spi_tx_data[2] = 0;
	spi_tx_data[3] = 0;
	spi_tx_data[4] = buffer[0];
	spi_tx_data[5] = buffer[1];

	spi_transfer(CPLD_PROGRAM_NO_READ, spi_tx_data, 6);

	if(tries < 10)
		goto verify;
	else
	{
		program_flash_exit(CPLD_PROG_ERROR);
	}

	tries++;
#endif
    return 2;
}

static void flash_program_done(void)
{
	/*Program DONE*/
	transmit(ISC_PROGRAM_DONE);
	delay_us(200);

	/* Read Status Register*/
	uint8_t status = read_status_reg();
	if ((Tst_bits(status, REG_BUSY)) || (Tst_bits(status, REG_FAIL)))
	{
		// TODO
	}

	delay_us(10);

	/* refresh*/
	transmit(LSC_REFRESH);

	program_flash_exit(CPLD_PROG_OK);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
size_t cpld_update_data_recieved(const uint8_t * buffer, size_t length)
{

	switch (pcpld->jed_file_portion)
	{
	case CONFIGURATION_FLASH_START:
		return configuration_start(buffer, length);
	case CONFIGURATION_FLASH_PROGRAM:
		return write_page(buffer, length);
	case WRITE_FEATURE_ROW:
		return write_feature_row(buffer, length);
	case WRITE_FEABITS:
		return write_fea_bits(buffer, length);
	default:
        return 0;
	}
	wdt_restart(WDT);
}

void lattice_firmware_update(void)
{
	/* Wait until we can grab the USB slave & SPI bus*/
	suspend_all_task();
	wdt_restart(WDT);
	cpld_reset();

	// TODO cpld_disable_interrupts();

	/*If it's the first time we program the cpld we need to slow the speed down*/
//	if(cpld_programmed == false)
//	{
	spi_set_baudrate_div(SPI0, 0,
			(sysclk_get_peripheral_hz() / SPI_SLOW_SPEED));
//	}

	/*Allocate memory for CPLD programming structure, this is freed in program_flash_exit*/
	pcpld = (CPLD*) malloc(sizeof(CPLD));
	if (pcpld == NULL)
	{
		debug_print(
				"ERROR: Could not allocate memory for CPLD programming.\r\n");
		return;
	}

	toggle_program_pin();
	delay_us(50);

#if 0	/* not needed*/
	if (check_device_id())
		goto stop;
#endif

	transmit(ISC_ENABLE_X);
	delay_us(50);

	/*earse_flash*/
	transmit(ISC_ERASE_CF_UFM_FR(EARSE_CONFIG_ONLY));
	delay_ms(500);

	/*Wait while erasing*/
	check_busy();
	delay_us(50);

	/* Read Status Register*/
	uint8_t status = read_status_reg();
	if ((Tst_bits(status, REG_BUSY)) || (Tst_bits(status, REG_FAIL)))
	{
		/* exit*/
		transmit(ISC_DISABLE);
		goto stop;
	}

	load_lattice_jed = 1; /* let USB sof know all data coming in is for jed files uploading*/

	pcpld->jed_file_portion = CONFIGURATION_FLASH_START; /*Setup the state machine*/
	respond_to_host(CPLD_PROG_OK);
	return;

	stop: program_flash_exit(CPLD_PROG_ERROR);
}

