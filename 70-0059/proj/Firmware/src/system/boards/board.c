/**
 * @file
 *
 * @brief Text description here . . .
 *
 */
#include "board.h"
#include "soft_i2c.h"
#include "slots.h"
#include "Debugging.h"
#include "25lc1024.h"
// #include "hexapod.h"
#include "otm.h"
#include "cpld.h"
#include "ftdi.h"
#include "helper.h"

#include "string.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
enum board_types board_type;
char usb_serial_number[USB_DEVICE_GET_SERIAL_NAME_LENGTH] = "UNPROG-SERIAL-NUM";
Boards board;

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static void get_board_type(void);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/**
 * @brief Get the type of board from on board EEPROM.
 */
// MARK:  SPI Mutex Required
static void get_board_type(void)
{
	eeprom_25LC1024_read(EEPROM_BOARD_INFO_ADDRESS, 2, (uint8_t *) &board_type);

	if(board_type >= END_BOARD_TYPES)
		board_type = BOARD_NOT_PROGRAMMED;

	if (board_type == BOARD_NOT_PROGRAMMED)
	{
		debug_print(
				"WARNING: Must be new EEPROM writing board type to NO_BOARD.");
		//	set_board_type(NO_BOARD);
		board_type = NO_BOARD;
	}
}

// MARK:  SPI Mutex Required
static void get_board_serial_number(void)
{
    eeprom_25LC1024_read(EEPROM_BOARD_INFO_ADDRESS + 2, USB_DEVICE_GET_SERIAL_NAME_LENGTH, (uint8_t*)usb_serial_number);

	// Ensuring that the buffer is null-terminated.
	usb_serial_number[USB_DEVICE_GET_SERIAL_NAME_LENGTH - 1] = '\0';
	cstring_to_uppercase(usb_serial_number);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
// MARK:  SPI Mutex Required
void board_save_info_eeprom(void)
{
	board.save.board_data_type = BOARD_DATA_TYPE;

	/*Calculate the address*/
	uint32_t eeprom_page_address = BOARD_SAVE_EEPROM_START;

	// save it to eeprom
	eeprom_25LC1024_write(eeprom_page_address, sizeof(Board_save) ,
			(uint8_t*) &board.save);
}

// MARK:  SPI Mutex Required
void board_get_info_eeprom(void)
{
	/* Reset the structure*/
	memset(&board.save, 0, sizeof(Board_save));

	/*Calculate the address*/
	uint32_t eeprom_page_address = BOARD_SAVE_EEPROM_START;

	eeprom_25LC1024_read(eeprom_page_address, sizeof(Board_save),
			(uint8_t*) &board.save);
}

/**
 * @brief Power must be turned on before anything else when stepper cards are present
 * because we found the stepper drive gets destroyed.
 * @param state
 */
void main_power_init(void)
{
	/* this controls the high voltage to the system*/
	set_pin_as_output(PIN_POWER_EN, PIN_STATE_ON);
}

void main_power( bool state)
{
	// this controls the high voltage to the system
	pin_state(PIN_POWER_EN, state);
}

/**
 * @brief This function must be called in init.  It gathers board information and configures
 * 		hardware and starts all tasks
 */
// MARK:  SPI Mutex Required
void board_init(void)
{
	board.error = 0;
	get_board_type();
	get_board_serial_number();
	board_get_info_eeprom();
	board.dim_bound = DEFAULT_SYSTEM_DIM_VALUE;
	board.clpd_em_stop = 0;
	board.send_log_ready = false;

	/*Keep for older not modified boards rev 003*/
	main_power_init();

	delay_ms(100);//100

	/* Get the board type and do it's initialization*/
	switch (board_type)
	{
	case OTM_BOARD_REV_002:
		otm_init();
		cpld_start_slot_cards();
		init_slots();
		break;

	case MCM6000_REV_002:
		ftdi_init();
		/*No slot card detection*/
		slots[SLOT_1].card_type = ST_HC_Stepper_type;
		slots[SLOT_2].card_type = ST_HC_Stepper_type; //ST_HC_Stepper_type;
		slots[SLOT_3].card_type = ST_HC_Stepper_type; //Shutter_type;
		slots[SLOT_4].card_type = ST_HC_Stepper_type;
		slots[SLOT_5].card_type = NO_CARD_IN_SLOT;
		slots[SLOT_6].card_type = NO_CARD_IN_SLOT;
		slots[SLOT_7].card_type = NO_CARD_IN_SLOT;

		/* Need to enable card slot here so we can do cable checking*/
		cpld_start_slot_cards();
		init_slots();
		break;

	case MCM6000_REV_003:
	case MCM6000_REV_004:
	case MCM_41_0117_001:	// CPLD 7000 new version
	case MCM_41_0134_001:   // MCM 301, revision 1
		ftdi_init();
		get_slot_types();
		cpld_start_slot_cards();
		init_slots();
		break;

	case MCM_41_0117_RT1064: // RT1064 USB CDC and HID host
		get_slot_types();
		cpld_start_slot_cards();
		init_slots();
		break;

	case HEXAPOD_BOARD_REV_001:
		cpld_start_slot_cards();
		// hexapod_init();
		break;

	case NO_BOARD:
		break;

	case BOARD_NOT_PROGRAMMED:
		break;

	default:
		break;
	}

	/* start the stepper synchronized motion controller task*/
	// stepper_synchronized_motion_init();
}

/**
 * @brief Sets the type of board we are using
 * @param board_type	A list of boards is in board.h.  Any new boards or revisions should be added there
 * @return 0 everything went well, 1 if could not set board type
 */
// MARK:  SPI Mutex Required
void set_board_type(uint16_t type)
{
	suspend_all_task();
	board_type = type;

	/*Erase all slot info*/
	for (uint8_t slot = 0; slot < NUMBER_OF_BOARD_SLOTS; ++slot)
	{
		eeprom_25LC1024_clear_page(SLOT_EEPROM_ADDRESS(slot, 0));
		delay_ms(20);
		eeprom_25LC1024_clear_page(SLOT_EEPROM_ADDRESS(slot, 1));
		delay_ms(20);
	}

	eeprom_25LC1024_write(EEPROM_BOARD_INFO_ADDRESS, 2, (uint8_t*) &board_type);
	delay_ms(20);
}

/**
 * Sets the serial number for the board.
 * The board will have to be restarted, but only after the reboot.
 * \param[in]   p_sn Pointer to the serial number of length USB_DEVICE_GET_SERIAL_NAME_LENGTH.
 */
// MARK:  SPI Mutex Required
void set_board_serial_number(char * const p_sn)
{
    // Copy everything into the serial number buffer.
    suspend_all_task();
    memcpy(usb_serial_number, p_sn, USB_DEVICE_GET_SERIAL_NAME_LENGTH);
	
	// Guarantee that the serial number is null-terminated.
	usb_serial_number[USB_DEVICE_GET_SERIAL_NAME_LENGTH - 1] = '\0';
	cstring_to_uppercase(usb_serial_number);

    // Write to board, wait for write, then restart the board.
    eeprom_25LC1024_write(EEPROM_BOARD_INFO_ADDRESS + 2, USB_DEVICE_GET_SERIAL_NAME_LENGTH, (uint8_t*)usb_serial_number);
    delay_ms(30);
}

void restart_board(void)
{
	/* reset the processor*/
#define 	RSTC_KEY		0xA5000001
	((Rstc *) RSTC)->RSTC_CR = RSTC_KEY;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
void set_firmware_load_count(void)
{
	uint32_t StatusFlg;
	uint32_t BootCount[1];
	StatusFlg = flash_read_user_signature(BootCount, 1);
	delay_ms(10);
	BootCount[0] = 105;
	StatusFlg = flash_erase_user_signature();
	delay_ms(50);
	StatusFlg = flash_write_user_signature(BootCount, 1);
	restart_board();
}
#pragma GCC diagnostic pop
