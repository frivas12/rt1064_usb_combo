/**
 * @file hid_in.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_USB_HOST_HID_IN_H_
#define SRC_SYSTEM_DRIVERS_USB_HOST_HID_IN_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
typedef enum
{
	CTL_AXIS = 0,
	CTL_BTN_SELECT,				// 1
	CTL_BTN_SELECT_TOGGLE, 		// 2
	CTL_BTN_TOGGLE_DISABLE_MOD,	// 3
	CTL_BTN_DISABLE_MOD,		// 4
	CTL_BTN_TOGGLE_DISABLE_DEST,// 5
	CTL_BTN_DISABLE_DEST, 		// 6
	CTL_BTN_JOG_CW,				// 7
	CTL_BTN_JOG_CCW, 			// 8
	CTL_BTN_POS_TOGGLE, 		// 9
	CTL_BTN_HOME,				// 10
	CTL_BTN_POS_1,				// 11
	CTL_BTN_POS_2,				// 12
	CTL_BTN_POS_3,				// 13
	CTL_BTN_POS_4,				// 14
	CTL_BTN_POS_5,				// 15
	CTL_BTN_POS_6,				// 16
	CTL_BTN_POS_7,				// 17
	CTL_BTN_POS_8,				// 18
	CTL_BTN_POS_9,				// 19
	CTL_BTN_POS_10, 			// 20
	CTL_BTN_SPEED,				// 21
	CTL_BTN_SPEED_TOGGLE		// 22
} Control_Modes;

typedef struct
{
    // HID Signature
    uint16_t idVendor; 						    ///> Vendor ID (assigned by the USB-IF).
    uint16_t idProduct; 					    ///> Product ID (assigned by the manufacturer).

    // Target HID Control (for manipulation)
    uint8_t modify_control_port;                ///> Target HID control (port no.) for this HID control.  Up to USB_NUMDEVICES.
    uint16_t modify_control_ctl_num;            ///> Target HID control (ctrl id) for this HID control.  Up to MAX_NUM_CONTROLS/

    // Dispatch Destinations
    uint8_t destination_slot;	                ///> Bitfield indicating which slot card receives a dispatched message.
                                                //   (0000 0100 0000 0101) slot 1 & 3 selected and USB port 2 for device like CDC
    uint8_t destination_bit;                    ///> Used to identify which device to interact with when a slot card drives multiple devices.
    uint8_t destination_port;                   ///> \warning Unused! Up to USB_NUMDEVICES.
    uint8_t destination_virtual;                ///> slot_num indicating which slot number, if any, is represented by this control in the synchronized motion sub-system.

    uint8_t speed_modifier;                     // The modifier for the maximum speed.

    uint8_t reverse_dir;                        ///> Boolean indicating if the direction is reversed.
    uint8_t dead_band;                          /// \warning Is the deadband code working correctly?

    Control_Modes mode;
    bool control_disable;	                    // used to disable just a control on the joystick
} Hid_Mapping_control_in;

/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void * hid_mapping_control_in_serialize_apt(const Hid_Mapping_control_in * obj, void * dest, size_t length);
const void * hid_mapping_control_in_deserialize_apt(Hid_Mapping_control_in * obj, const void * dest, size_t length);
void * hid_mapping_control_in_serialize_eeprom(const Hid_Mapping_control_in * obj, void * dest, size_t length);
const void * hid_mapping_control_in_deserialize_eeprom(Hid_Mapping_control_in * obj, const void * dest, size_t length);

void hid_mapping_control_in_persist(uint8_t port);

/**
 * This function defines the control types that can
 * be mutated by mutation controls.
 */
bool control_mode_is_mutable(Control_Modes mode);

/**
 * This function defines the control types that can
 * be mutated by mutation controls.
 */
bool control_mode_is_mutating(Control_Modes mode);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_USB_HOST_HID_IN_H_ */
