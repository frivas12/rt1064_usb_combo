/**
 * @file usb_hub.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_USB_HOST_USB_HUB_H_
#define SRC_SYSTEM_DRIVERS_USB_HOST_USB_HUB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "usb_host.h"

/****************************************************************************
 * Defines
 ****************************************************************************/
#define USB_DESCRIPTOR_HUB                      0x09 // Hub descriptor type

// Hub Requests
#define bmREQ_CLEAR_HUB_FEATURE                 USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_DEVICE
#define bmREQ_CLEAR_PORT_FEATURE                USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER
#define bmREQ_CLEAR_TT_BUFFER                   USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER
#define bmREQ_GET_HUB_DESCRIPTOR                USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_DEVICE
#define bmREQ_GET_HUB_STATUS                    USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_DEVICE
#define bmREQ_GET_PORT_STATUS                   USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER
#define bmREQ_RESET_TT                          USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER
#define bmREQ_SET_HUB_DESCRIPTOR                USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_DEVICE
#define bmREQ_SET_HUB_FEATURE                   USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_DEVICE
#define bmREQ_SET_PORT_FEATURE                  USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER
#define bmREQ_GET_TT_STATE                      USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER
#define bmREQ_STOP_TT                           USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_OTHER

// Hub Class Requests
#define HUB_REQUEST_CLEAR_TT_BUFFER             8
#define HUB_REQUEST_RESET_TT                    9
#define HUB_REQUEST_GET_TT_STATE                10
#define HUB_REQUEST_STOP_TT                     11

// Hub Features
#define HUB_FEATURE_C_HUB_LOCAL_POWER           0
#define HUB_FEATURE_C_HUB_OVER_CURRENT          1
#define HUB_FEATURE_PORT_CONNECTION             0
#define HUB_FEATURE_PORT_ENABLE                 1
#define HUB_FEATURE_PORT_SUSPEND                2
#define HUB_FEATURE_PORT_OVER_CURRENT           3
#define HUB_FEATURE_PORT_RESET                  4
#define HUB_FEATURE_PORT_POWER                  8
#define HUB_FEATURE_PORT_LOW_SPEED              9
#define HUB_FEATURE_C_PORT_CONNECTION           16
#define HUB_FEATURE_C_PORT_ENABLE               17
#define HUB_FEATURE_C_PORT_SUSPEND              18
#define HUB_FEATURE_C_PORT_OVER_CURRENT         19
#define HUB_FEATURE_C_PORT_RESET                20
#define HUB_FEATURE_PORT_TEST                   21
#define HUB_FEATURE_PORT_INDICATOR              22

// Hub Port Test Modes
#define HUB_PORT_TEST_MODE_J                    1
#define HUB_PORT_TEST_MODE_K                    2
#define HUB_PORT_TEST_MODE_SE0_NAK              3
#define HUB_PORT_TEST_MODE_PACKET               4
#define HUB_PORT_TEST_MODE_FORCE_ENABLE         5

// Hub Port Indicator Color
#define HUB_PORT_INDICATOR_AUTO                 0
#define HUB_PORT_INDICATOR_AMBER                1
#define HUB_PORT_INDICATOR_GREEN                2
#define HUB_PORT_INDICATOR_OFF                  3

// Hub Port Status Bitmasks
#define bmHUB_PORT_STATUS_PORT_CONNECTION       0x0001
#define bmHUB_PORT_STATUS_PORT_ENABLE           0x0002
#define bmHUB_PORT_STATUS_PORT_SUSPEND          0x0004
#define bmHUB_PORT_STATUS_PORT_OVER_CURRENT     0x0008
#define bmHUB_PORT_STATUS_PORT_RESET            0x0010
#define bmHUB_PORT_STATUS_PORT_POWER            0x0100
#define bmHUB_PORT_STATUS_PORT_LOW_SPEED        0x0200
#define bmHUB_PORT_STATUS_PORT_HIGH_SPEED       0x0400
#define bmHUB_PORT_STATUS_PORT_TEST             0x0800
#define bmHUB_PORT_STATUS_PORT_INDICATOR        0x1000

// Hub Port Status Change Bitmasks (used one byte instead of two)
#define bmHUB_PORT_STATUS_C_PORT_CONNECTION     0x0001
#define bmHUB_PORT_STATUS_C_PORT_ENABLE         0x0002
#define bmHUB_PORT_STATUS_C_PORT_SUSPEND        0x0004
#define bmHUB_PORT_STATUS_C_PORT_OVER_CURRENT   0x0008
#define bmHUB_PORT_STATUS_C_PORT_RESET          0x0010

// Hub Status Bitmasks (used one byte instead of two)
#define bmHUB_STATUS_LOCAL_POWER_SOURCE         0x01
#define bmHUB_STATUS_OVER_CURRENT               0x12

// Hub Status Change Bitmasks (used one byte instead of two)
#define bmHUB_STATUS_C_LOCAL_POWER_SOURCE       0x01
#define bmHUB_STATUS_C_OVER_CURRENT             0x12

// Hub Port Configuring Substates
#define USB_STATE_HUB_PORT_CONFIGURING          0xb0
#define USB_STATE_HUB_PORT_POWERED_OFF          0xb1
#define USB_STATE_HUB_PORT_WAIT_FOR_POWER_GOOD  0xb2
#define USB_STATE_HUB_PORT_DISCONNECTED         0xb3
#define USB_STATE_HUB_PORT_DISABLED             0xb4
#define USB_STATE_HUB_PORT_RESETTING            0xb5
#define USB_STATE_HUB_PORT_ENABLED              0xb6

// Additional Error Codes
#define HUB_ERROR_PORT_HAS_BEEN_RESET           0xb1

// The bit mask to check for all necessary state bits
#define bmHUB_PORT_STATUS_ALL_MAIN              ((0UL | bmHUB_PORT_STATUS_C_PORT_CONNECTION | bmHUB_PORT_STATUS_C_PORT_ENABLE | bmHUB_PORT_STATUS_C_PORT_SUSPEND | bmHUB_PORT_STATUS_C_PORT_RESET) << 16) | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_ENABLE | bmHUB_PORT_STATUS_PORT_CONNECTION | bmHUB_PORT_STATUS_PORT_SUSPEND)

// Bit mask to check for DISABLED state in HubEvent::bmStatus field
#define bmHUB_PORT_STATE_CHECK_DISABLED         (0x0000 | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_ENABLE | bmHUB_PORT_STATUS_PORT_CONNECTION | bmHUB_PORT_STATUS_PORT_SUSPEND)

// Hub Port States
#define bmHUB_PORT_STATE_DISABLED               (0x0000 | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_CONNECTION)

// Hub Port Events
#define bmHUB_PORT_EVENT_CONNECT                (((0UL | bmHUB_PORT_STATUS_C_PORT_CONNECTION) << 16) | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_CONNECTION)
#define bmHUB_PORT_EVENT_DISCONNECT             (((0UL | bmHUB_PORT_STATUS_C_PORT_CONNECTION) << 16) | bmHUB_PORT_STATUS_PORT_POWER)
#define bmHUB_PORT_EVENT_RESET_COMPLETE         (((0UL | bmHUB_PORT_STATUS_C_PORT_RESET) << 16) | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_ENABLE | bmHUB_PORT_STATUS_PORT_CONNECTION)

#define bmHUB_PORT_EVENT_LS_CONNECT             (((0UL | bmHUB_PORT_STATUS_C_PORT_CONNECTION) << 16) | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_CONNECTION | bmHUB_PORT_STATUS_PORT_LOW_SPEED)
#define bmHUB_PORT_EVENT_LS_RESET_COMPLETE      (((0UL | bmHUB_PORT_STATUS_C_PORT_RESET) << 16) | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_ENABLE | bmHUB_PORT_STATUS_PORT_CONNECTION | bmHUB_PORT_STATUS_PORT_LOW_SPEED)
#define bmHUB_PORT_EVENT_LS_PORT_ENABLED        (((0UL | bmHUB_PORT_STATUS_C_PORT_CONNECTION | bmHUB_PORT_STATUS_C_PORT_ENABLE) << 16) | bmHUB_PORT_STATUS_PORT_POWER | bmHUB_PORT_STATUS_PORT_ENABLE | bmHUB_PORT_STATUS_PORT_CONNECTION | bmHUB_PORT_STATUS_PORT_LOW_SPEED)

/****************************************************************************
 * Public Data
 ****************************************************************************/
typedef struct
{
	uint8_t bDescriptorLength;
	uint8_t bDescriptorType;
	uint8_t bNumberOfPorts;
	uint8_t wHubCharacteristics;
	uint8_t wHubCharacteristicsh;
	uint8_t bPowerOn2PwrGood;
	uint8_t bHubControlCurrent;
	uint8_t DeviceRemovable;
	uint8_t PortPwrCtrlMask;
} HubDescriptor;

typedef struct
{
	union
	{

		struct
		{
			uint16_t bmStatus; // port status bits
			uint16_t bmChange; // port status change bits
		};
		uint32_t bmEvent;
		uint8_t evtBuff[4];
	};
} HubEvent;

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
uint8_t GetHubDescriptor(uint8_t address);
uint8_t Hub_ports_pwr(void);
uint8_t Poll_hub(void);


#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_USB_HOST_USB_HUB_H_ */
