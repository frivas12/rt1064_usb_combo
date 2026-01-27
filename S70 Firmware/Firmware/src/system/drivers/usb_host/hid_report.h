// hid_report.h

#ifndef SRC_SYSTEM_DRIVERS_USB_HOST_HID_REPORT_H_
#define SRC_SYSTEM_DRIVERS_USB_HOST_HID_REPORT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
// (sbenish)  With the 1 byte header, a 256-byte page can only hold
//            x15 in controls (16 bytes each).
#define MAX_NUM_CONTROLS			15
#define HID_MAX_BYTES				24 //16
#define HID_MAX_NESTING				3  // Allows for a HID report with a collection depth of 3 (Application collection, Secondary collection, Tertiary collection)

#define TYPE_INPUT			0
#define TYPE_OUTPUT			1
#define TYPE_NOT_SET		255

#define RCODE_REPORT_LOGIC_ERROR	0xFF

/****************************************************************************
 * Public Data
 ****************************************************************************/
typedef struct
{
	uint32_t usage;
	uint8_t item_bits;
	uint8_t report_size; /*In bits*/
	uint8_t bit_position;
	uint8_t byte_position;
	int16_t logical_min;	/* maybe negative*/
	int16_t logical_max;
	bool first_time;	/*Flag to let us know this is the first value from the
	 	 	 	 	 	 control.  In controls such as dials where we calculate a
	 	 	 	 	 	 delta from the previous value, we need to know the first
	 	 	 	 	 	 time in so we don't calculate a delta based on an incorrect
	 	 	 	 	 	 previous value*/
	int16_t prev_value;
} Hid_Control_info;

typedef struct
{
//	joystick_modes mode;
	uint8_t active_slot_toggle;
	uint8_t num_of_input_bits;
	uint8_t num_of_input_bytes;
	uint8_t num_of_input_controls;
	uint8_t num_of_input_controls_initialized;
	Hid_Control_info hid_in_controls[MAX_NUM_CONTROLS];

	uint8_t num_of_output_bits;
	uint8_t num_of_output_bytes;
	uint8_t num_of_output_controls;
	Hid_Control_info hid_out_controls[MAX_NUM_CONTROLS];

	uint8_t hid_buf[HID_MAX_BYTES]; /* this is the buffer where all the data read come into*/
	uint8_t hid_prev_buf[HID_MAX_BYTES]; /* this is the compare buffer*/

    int8_t hid_in_ep;
    int8_t hid_out_ep;

	uint8_t slot_select;	/* ShuttelExpress Only used when mode = JOYSTICK_SLOT_SELECT_MODE*/
} Hid_Data;

typedef struct
{
	/*These are the parts of the HID report we parse at on time and move along
	 * until the end of the report*/
	uint8_t first_byte;
	uint8_t bTag;
	uint8_t bType;
	uint8_t bSize;

	uint8_t second_byte;
	uint8_t third_byte;
	uint8_t fourth_byte;

	uint8_t usage_min;
	uint8_t usage_max;
	uint16_t usage_page;
	uint8_t num_of_usages;	// Starts at 0 and increments each time a usage is defined.
	uint32_t usage[MAX_NUM_CONTROLS];
	uint8_t usage_type;
	uint8_t item_bits;
	int16_t logical_min;
	int16_t logical_max;

	uint8_t bit_position_in;
	uint8_t bit_position_out;
	uint8_t report_size; /* in bits*/
	uint8_t report_count;
	uint8_t usage_count;
} Report_Items;

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
uint8_t HID_get_report(uint8_t address, uint16_t lengthOfDescriptor);
uint16_t HID_get_usage_page(const Hid_Control_info * const p_control);
uint16_t HID_get_usage_id(const Hid_Control_info * const p_control);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_USB_HOST_HID_REPORT_H_ */
