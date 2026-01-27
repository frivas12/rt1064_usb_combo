/* HID support header */
#ifndef _HID_h_
#define _HID_h_

#include "hid_report.h"
#include "hid_in.h"
#include "hid_out.h"
#include "UsbCore.h"

#ifdef __cplusplus
extern "C"
{
#endif


/****************************************************************************
 * Defines
 ****************************************************************************/

/* HID constants. Not part of chapter 9 */
/* Class-Specific Requests */
#define HID_REQUEST_GET_REPORT      0x01
#define HID_REQUEST_GET_IDLE        0x02
#define HID_REQUEST_GET_PROTOCOL    0x03
#define HID_REQUEST_SET_REPORT      0x09
#define HID_REQUEST_SET_IDLE        0x0A
#define HID_REQUEST_SET_PROTOCOL    0x0B

/* Class Descriptor Types */
#define HID_DESCRIPTOR_HID      0x21
#define HID_DESCRIPTOR_REPORT   0x22
#define HID_DESRIPTOR_PHY       0x23

/* Protocol Selection */
#define BOOT_PROTOCOL   0x00
#define RPT_PROTOCOL    0x01
/* HID Interface Class Code */
#define HID_INTF                    0x03
/* HID Interface Class SubClass Codes */
#define BOOT_INTF_SUBCLASS          0x01
/* HID Interface Class Protocol Codes */
#define HID_PROTOCOL_NONE           0x00
#define HID_PROTOCOL_KEYBOARD       0x01
#define HID_PROTOCOL_MOUSE          0x02

//Every HID class device identifies itself with a report descriptor. A Report
//	descriptor describes each piece of data that the device generates and
//	what the data is actually measuring. A parser is needed to decode the
//	content of report descriptor.
//
//	Report descriptors are composed of pieces of information. Each piece of
//	information is called an Item.
//	HID Item Header
//
//	---------------------------------------------------------
//	|  7   |  6   |  5   |  4   |  3   |  2   |  1   |  0   |
//	|            Tag            |    Type     |    Size     |
//	---------------------------------------------------------

#define HID_ITEM_TYPE_MAIN              0
#define HID_ITEM_TYPE_GLOBAL            1
#define HID_ITEM_TYPE_LOCAL             2
#define HID_ITEM_TYPE_RESERVED          3

// HID report descriptor main item tags
#define HID_MAIN_ITEM_TAG_INPUT                 8
#define HID_MAIN_ITEM_TAG_OUTPUT                9
#define HID_MAIN_ITEM_TAG_FEATURE               11
#define HID_MAIN_ITEM_TAG_BEGIN_COLLECTION      10
#define HID_MAIN_ITEM_TAG_END_COLLECTION        12

// HID report descriptor global item tags
#define HID_GLOBAL_ITEM_TAG_USAGE_PAGE          0
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MINIMUM     1
#define HID_GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM     2
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MINIMUM    3
#define HID_GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUM    4
#define HID_GLOBAL_ITEM_TAG_UNIT_EXPONENT       5
#define HID_GLOBAL_ITEM_TAG_UNIT                6
#define HID_GLOBAL_ITEM_TAG_REPORT_SIZE         7
#define HID_GLOBAL_ITEM_TAG_REPORT_ID           8
#define HID_GLOBAL_ITEM_TAG_REPORT_COUNT        9
#define HID_GLOBAL_ITEM_TAG_PUSH                10
#define HID_GLOBAL_ITEM_TAG_POP                 11

// HID report descriptor local item tags
#define HID_LOCAL_ITEM_TAG_USAGE                0
#define HID_LOCAL_ITEM_TAG_USAGE_MINIMUM        1
#define HID_LOCAL_ITEM_TAG_USAGE_MAXIMUM        2
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_INDEX     3
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_MINIMUM   4
#define HID_LOCAL_ITEM_TAG_DESIGNATOR_MAXIMUM   5
#define HID_LOCAL_ITEM_TAG_STRING_INDEX         7
#define HID_LOCAL_ITEM_TAG_STRING_MINIMUM       8
#define HID_LOCAL_ITEM_TAG_STRING_MAXIMUM       9
#define HID_LOCAL_ITEM_TAG_DELIMITER            10



//Usage pages
//section 3 of HID usage tables from usb.org
#define HID_USAGE_PAGE_GENERIC_DESKTOP		0x01
#define HID_USAGE_PAGE_SIMULATION			0x02
#define HID_USAGE_PAGE_VR					0x03
#define HID_USAGE_PAGE_SPORT				0x04
#define HID_USAGE_PAGE_GAME					0x05
#define HID_USAGE_PAGE_GENERIC_DEVICE		0x06
#define HID_USAGE_PAGE_KEYBOARD_KEYPAD		0x07
#define HID_USAGE_PAGE_LED					0x08
#define HID_USAGE_PAGE_BUTTON				0x09

//Generic desktop Usages
// Section 4 of HID usage tables from usb.org
#define HID_USAGE_UNDEFINED					0x00
#define HID_USAGE_POINTER					0x01
#define HID_USAGE_MOUSE						0x02
#define HID_USAGE_RESERVED					0x03
#define HID_USAGE_JOYSTICK					0x04
#define HID_USAGE_GAME_PAD					0x05
#define HID_USAGE_KEYBOARD					0x06
#define HID_USAGE_KEYPAD					0x07
#define HID_USAGE_MULTIAXIS_CONTROLLER		0x08
#define HID_USAGE_TABLET_PC_SYSTEM			0x09
#define HID_USAGE_X							0x30
#define HID_USAGE_Y							0x31
#define HID_USAGE_Z							0x32
#define HID_USAGE_RX						0x33
#define HID_USAGE_RY						0x34
#define HID_USAGE_RZ						0x35
#define HID_USAGE_SLIDER					0x36
#define HID_USAGE_DIAL						0x37
#define HID_USAGE_WHEEL						0x38
#define HID_USAGE_VX						0x40
#define HID_USAGE_VY						0x41
#define HID_USAGE_VZ						0x42

// LED Page
// Section 11 of HID usage tables from usb.org
#define HID_USAGE_INDICATOR_RED				0x48
#define HID_USAGE_INDICATOR_GREEN			0x49
#define HID_USAGE_INDICATOR_AMBER			0x4A
#define HID_USAGE_GENERIC_INDICATOR			0x4B

//Usage types
#define HID_USAGE_TYPE_DEFAULT			0x00
#define HID_USAGE_TYPE_LINEAR			0x01
#define HID_USAGE_TYPE_ON_OFF			0x02
#define HID_USAGE_TYPE_MOMENTARY		0x03
#define HID_USAGE_TYPE_ONE_SHOT			0x04
//#define Retrigger_Control		0x05
//#define Selector				0x06
//#define	Static_Value			0x07
//#define Static_Flag				0x08
//#define Dynamic_Flag			0x09
//#define Dynamic_Value			0x0A
//#define Named_Array				0x0B
//#define Collection_Application	0x0C
//#define Colleciton_Physical		0x0D
//#define Collection_Logical		0x0E
//#define Usage_Switch			0x0F
//#define Usage_Modifier			0x10

// Collection Types
#define PHYSICAL 				0x00
#define APPLICATION 			0x01
#define LOGICAL 				0x02
#define REPORT 					0x03
#define NAMED_ARRAY 			0x04
#define USAGE_SWITCH 			0x05
#define USAGE_MODIFIER 			0x06
#define USAGE_RESERVED 			0x07

#define ALL_VID_PID					0
#define SYNCHRONIZED_MOTION_BIT		0x80

/*Bits for item parts*/
#define ITEM_DATA_CONTST		1<<0
#define ITEM_ARRAY_VAR			1<<1
#define ITEM_ABS_REL			1<<2
#define ITEM_WRAP_NOWRAP		1<<3
#define ITEM_LIN_NONLIN			1<<4
#define ITEM_PREF_NOPREF		1<<5
#define ITEM_NONULL_NULL		1<<6
#define ITEM_NOVOL_VOLATILE		1<<7



/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void get_hid_device_info(uint8_t address, uint8_t* response_buffer);
int get_hid_control_info(uint8_t address, bool isOut, uint8_t control_index,
                         uint8_t* response_buffer);
#ifdef __cplusplus
}
#endif

#endif // _HID_h_

