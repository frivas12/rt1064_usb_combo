// slots.h

#ifndef SRC_SYSTEM_BOARDS_SLOTS_H_
#define SRC_SYSTEM_BOARDS_SLOTS_H_

#include "compiler.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "usb_slave.h"
#include "slot_types.h"
#include "device_detect.h"
#include "defs.h"
#include "slot_nums.h"
#include "slot_types.h"
#include "pnp_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
#define NUMBER_OF_BOARD_SLOTS	((7+1))	// 7 slots plus the half slot

#define SLOT_CS(slot)		((slot*2) + 97)

#define STATIC_CARD_SLOT_MASK	0x8000

#define USER_SLOT_NAME_LENGTH   16

#define EM_STOP_ALL 	0x7F
#define SLOT_INIT_DELAY	300

/****************************************************************************
 * Public Data
 ****************************************************************************/
/*This data is saved to the last page in the slot EEPROM data*/
typedef struct  __attribute__((packed))
{
	/// \todo See below
	/// \warning Currently, the allowed-devices size is NOT checked against PAGE size, so overflow may occur.  Must fix in the future.
	uint8_t slot_data_type;
	slot_types static_card_type;				// set to END_CARD_TYPES if card detection is used and not static card slot.
	uint8_t allow_device_detection;				// Set to 0 to disable device detection
	char user_slot_name[USER_SLOT_NAME_LENGTH]; ///> Does NOT include the null terminator.
	uint64_t allowed_device_serial_number;		// The serial number of a one-wire device required for the slot to run.
												// If set to 0xFFFF000000000000, then any serial number is allowed.
	uint16_t num_allowed_devices;               ///> Number of device types in the array.
	/// \warning In memory, num_allowed_devices is followed by an array of allowed devices.
	/// DO NOT PUT ANYTHING ELSE BELOW num_allowed_devices!
} Slot_Save;

typedef struct
{
	SemaphoreHandle_t xSlot_Mutex;
	slot_types card_type;			// Slot type of the card (either detected or static).
	Device device;		/*This is storage for the device detected from the EEPROM on the device*/
	Slot_Save save;
	device_signature_t * p_allowed_devices;     ///> Pointer to an array of device signatures allowed by this slot.
												///  Exists for the lifetime of the system
												///  Signatures are needed if the slot card can drive more basic devices.
	bool em_stop;	// used to signal slot to stop, i.e. stepper stop stage goto idle
	volatile bool interrupt_flag_cpld;	// interrupt from cpld indicating pin change
	void (*p_interrupt_cpld_handler)(uint8_t slot, void *structure);
	volatile bool interrupt_flag_smio;	// interrupt from slot to uC indicating pin change
	void (*p_interrupt_smio_handler)(uint8_t slot, void *structure);
	pnp_status_t pnp_status;

	// Supervisor-controlled, mutex-independent variables.
	/**
	 * When set to 0xFF, it uses global dimming.
	 * When set between 0 and 100, it sets the brightness value.
	 */
	uint8_t slot_led_brightness;
} Slots;

extern Slots slots[NUMBER_OF_BOARD_SLOTS];


/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void slot_save_info_eeprom(slot_nums slot);
void slot_get_info_eeprom(slot_nums slot);
void get_slot_types(void);
void em_stop(uint8_t em_slots_bitset);
void init_slots(void);
uint8_t set_slot_type(slot_nums slot, slot_types slot_type);

/// Returns true if the device is compatible with the save-struct
bool device_is_compatible(const slot_types slot_type, const device_id_t device_id,
		const slot_nums slot_number);

/**
 * Used to initialize the slot synchronization ojects for slot cards.
 * If the syncrhonization objects are not initialized (like when the card type
 * is unknown), the system will hang.
 */
void init_slots_synchronization (void);

/**
 * Attempts to verify that a config loaded into the save buffer is correct.
 * If not, it will attempt to load in the default configuration.
 * \note This function only works for slot types with only 1 sub-device (steppers, servos, etc.).
 * 		 Call slot_process_multiple_config to handle slot types with multiple sub-devices.
 * \param[in]	slot_num Slot number of the device to verify.
 * \param[in]   saved_signature Signature of the device saved.
 * \param[out]	p_save_buffer Pointer to the save structure of the buffer.
 * \return true A valid configuration was loaded.  The driver may run.
 * \return false A problem occurred during configuration.  The driver may NOT run.
 */
bool slot_process_config(const slot_nums slot_num, const device_signature_t saved_signature, void * p_save_buffer, uint32_t verify_length, const CRC8_t saved_crc);

/**
 * Attempts to verify that a config loaded into the save buffer is correct.
 * If not, it will attempt to load in the default configuration.
 * \warning p_sub_device_array assumes that the save structure is at offset 0 in each sub-device's structure.
 *          Data corruption will occur otherwise.
 * \param[in]	slot_num Slot number of the device to verify.
 * \param[out]	p_sub_device_array Pointer to an array of sub-devices to configure.  Must enough sub-devices as declared by save structure.
 * \param[in] 	sub_device_structue_size Size of the structure of each sub-device.  Use the sizeof() operator on the struture.
 * \return true Valid configurations were loaded.  The driver may run.
 * \return false A problem occurred during configuration.  The driver may NOT run.
 */
bool slot_process_multiple_config(const slot_nums slot_num, void * const p_sub_device_array,
	const uint32_t sub_device_structure_size);

/**
 * Attempts to parse a generic APT commands on the slot (OW, device detect, etc.).
 * The caller must send data if this function returns true.
 * \param[in]		slot_num Slot number that this command points to.
 * \param[in]		p_msg USB Message
 * \param[out]		p_response_buffer Buffer to response data.
 * \param[out]		p_length Size of response to send.
 * \param[out]		p_send If a response should be sent.
 * \return true Message was handled.
 * \return false Message was not handles.
 */
bool slot_parse_generic_apt(const slot_nums slot_num, USB_Slave_Message * const p_msg, 
	uint8_t * const p_response_buffer, uint8_t * const p_length, bool * const p_send);

/**
 * Indicates if a device that runs off a certain card could possible run on another card.
 * \param[in]           card_type Type of the card (hardware).
 * \param[in]           signatures_card_type Default card type for a device.
 * \return true The card could drive a device that runs off the provided alternate card.
 * \return false The card could not drive a device that runs off the provided alternate card.
 */
bool slot_is_compatible(slot_types card_type, slot_types signatures_card_type);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_BOARDS_SLOTS_H_ */
