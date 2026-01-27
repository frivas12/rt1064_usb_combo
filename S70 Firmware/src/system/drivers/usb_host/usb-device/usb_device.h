// usb_device.h

#ifndef SRC_SYSTEM_DRIVERS_USB_HOST_USB_DEVICE_H_
#define SRC_SYSTEM_DRIVERS_USB_HOST_USB_DEVICE_H_

#include "hid_out.h"
#include "hid_report.h"
#include "usb_ch9.h"
#include "UsbCore.h"


#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
#define HUB_ADDRESS 1

#define USB_HOST_QUEUE_LENGTH 1

/* Device task states */
typedef enum {
    WAIT = 0,
    ENUMERATE, /* USB device just connected, so we will try to enumerate */
    POLL,
    POLLx
} usb_device_task_states;

/****************************************************************************
 * Public Data
 ****************************************************************************/
typedef struct {
    uint8_t epAddr;  // copy from endpoint descriptor. Bit 7 indicates direction
                     // ( ignored for control endpoints )
    uint8_t Attr;         // Endpoint transfer type.
    uint16_t MaxPktSize;  // Maximum packet size.
    uint8_t Interval;     // Polling interval in frames.
    uint8_t sndToggle;    // last toggle value, bitmask for HCTL toggle bits
    uint8_t rcvToggle;    // last toggle value, bitmask for HCTL toggle bits
    /* not sure if both are necessary */
} EpInfo;

typedef struct {
    //	volatile uint8_t Interval_cnt; 	// Polling interval counter.
    uint8_t bNumberOfPorts;
    uint8_t bmHubPre;
    bool PollEnable;
    bool bResetInitiated;

} HubInfo;

typedef struct {
    bool enumerated; /* Indicates if a device has been enumerated */
    bool lowspeed;   /* Indicates if a device is a low speed one */
    bool is_hub;     /* flag to let system know we have a HUB connected */
    USB_DEVICE_DESCRIPTOR dev_descriptor;
    USB_CONFIGURATION_DESCRIPTOR config_descriptor;
    USB_INTERFACE_DESCRIPTOR interface_descriptor;
    USB_HID_DESCRIPTOR hid_descriptor;
    USB_ENDPOINT_DESCRIPTOR ep_descriptor[MAX_NUM_EP];
    EpInfo ep_info[MAX_NUM_EP];
    Hid_Data hid_data;
    HubInfo hub;
    bool
        initialized; /* flag to let system know a device (joystick) has read the
                      status of all the controls. This is needed for controls
                      like toggle switches to know their initial state*/
} UsbDevice;

typedef struct {
    uint8_t address;
    bool lowspeed; /* Indicates if a device is a low speed one */
    uint8_t task_state;
} UsbNewDevice;

extern UsbDevice usb_device[USB_NUMDEVICES];
extern UsbNewDevice *pDev[USB_NUMDEVICES];

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void usb_device_add_device_task(UsbNewDevice* pNew_device);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_USB_HOST_USB_DEVICE_H_ */
