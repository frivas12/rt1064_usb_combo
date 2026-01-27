/**
 * @file hid_out.h
 *
 * @brief Functions for USB device task out data
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_USB_HOST_HID_OUT_H_
#define SRC_SYSTEM_DRIVERS_USB_HOST_HID_OUT_H_

#include <stddef.h>

#include "hid_report.h"

#ifdef __cplusplus
extern "C"
{
#endif


/****************************************************************************
 * Defines
 ****************************************************************************/

#define FADE_LED_LIMIT 1 	/* 0 for blink, 1 for fade*/
#define CURRENT_STORE_POS_NONE			0xFF

/*
Led modes
Stage 		(limit, disabled, stopped, moving)
Disable_all	(disable_all)
Toggle		(position 1 & position 2)
Position	(positions 1 - 10)

*/

/*Led modes*/
typedef enum {
	LED_STAGE,			// 0
	LED_DISABLE_ALL,	// 1
	LED_TOGGLE,			// 2
	LED_POSITION_1,		// 3
	LED_POSITION_2,		// 4
	LED_POSITION_3,		// 5
	LED_POSITION_4,		// 6
	LED_POSITION_5,		// 7
	LED_POSITION_6,		// 8
	LED_POSITION_7,		// 9
	LED_POSITION_8,		// 10
	LED_POSITION_9,		// 11
	LED_POSITION_10,	// 12
	LED_SPEED_SWITCH,	// 13
	LED_STAGE_INVERT,       // 14
} Led_modes;

/*Led colors ID*/
typedef enum {
	LED_ID_OFF,
	LED_ID_WHT,		// 1
	LED_ID_RED, 	// 2
	LED_ID_GRN, 	// 3
	LED_ID_BLU, 	// 4
	LED_ID_YEL, 	// 5
	LED_ID_AQU, 	// 6
	LED_ID_MGN, 	// 7
	LED_ID_WHT_DIM, // 8
	LED_ID_RED_DIM, // 9
	LED_ID_GRN_DIM, // 10
	LED_ID_BLU_DIM, // 11
	LED_ID_YEL_DIM, // 12
	LED_ID_AQU_DIM, // 13
	LED_ID_MGN_DIM 	// 14
} Led_color_id;

#define DIM_VALUE	0x15 // 0x20

/*Led colors*/
typedef enum {
	LED_OFF = 0,			// 0
	LED_WHT = 0xFFFFFF,		// 1
	LED_RED = 0xFF0000,		// 2
	LED_GRN = 0x00FF00,		// 3
	LED_BLU = 0x0000FF,		// 4
	LED_YEL = 0xFFFF00,		// 5
	LED_AQU = 0x00FFFF,		// 6
	LED_MGN = 0xFF00FF,		// 7
	LED_WHT_DIM =  DIM_VALUE << 16 | DIM_VALUE << 8 | DIM_VALUE,	// 0x202020,	//8
	LED_RED_DIM = DIM_VALUE << 16,	// 0x200000,	// 9
	LED_GRN_DIM = DIM_VALUE << 8,	// 0x002000,	// 10
	LED_BLU_DIM = DIM_VALUE,		// 0x000020,	// 11
	LED_YEL_DIM = DIM_VALUE << 16 | DIM_VALUE << 8, // 0x202000,	// 12
	LED_AQU_DIM = DIM_VALUE << 8 | DIM_VALUE,		// 0x002020,	// 13
	LED_MGN_DIM = DIM_VALUE << 16 | DIM_VALUE,		// 0x200020
} Led_color;

/****************************************************************************
 * Public Data
 ****************************************************************************/

typedef struct
{
	uint8_t type;					///> HID Usage Type (see HID usage tables from usb.org)

	// HID Signature
	uint16_t idVendor; 				///> Vendor ID (assigned by the USB-IF).
	uint16_t idProduct; 			///> Product ID (assigned by the manufacturer).

	// LED Controls
	Led_modes mode;					///> LED Operating Mode
	Led_color_id color_1_id;		///> Color for Alert (Disabled, limit, etc.).
	Led_color_id color_2_id;		///> Color for Ready.
	Led_color_id color_3_id;		///> Color for Operating (Moving steppers, etc.).

	// Dispatch Source
	uint8_t source_slots; /*What slot or USB port will source this control,
	 (0000 0100 0000 0101) slot 1 & 3 selected and USB port 2 for device like CDC*/
	uint8_t source_bit;				///> \warning Unused.
	uint8_t source_port;			///> \warning Unused. Up to USB_NUMDEVICES.
	uint8_t source_virtual;			///> \warning Unused.
} Hid_Mapping_control_out;

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
/**
 * Serializes the control_out map into the buffer.
 * \note If the length is less than 13, the no serialization will occur.
 * \return The buffer shifted by the number of bytes serialized to it.
 */
void * hid_mapping_control_out_serialize(const Hid_Mapping_control_out * obj, void * buffer, size_t length);

/**
 * Deserializes the control_out map from the buffer.
 * \note If the length is less than 13, the no serialization will occur.
 * \return The buffer shifted by the number of bytes deserialized from it.
 */
const void * hid_mapping_control_out_deserialize(Hid_Mapping_control_out * obj, const void * buffer, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_USB_HOST_HID_OUT_H_ */
