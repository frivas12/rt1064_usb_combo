// UsbCore.h

#ifndef SRC_DRIVERS_USB_HOST_USBCORE_H_
#define SRC_DRIVERS_USB_HOST_USBCORE_H_

/****************************************************************************
 * Defines
 ****************************************************************************/
//#error "Never include UsbCore.h directly; include Usb.h instead"
/* Common setup data constant combinations  */
#define bmREQ_GET_DESCR     USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_STANDARD|USB_SETUP_RECIPIENT_DEVICE     //get descriptor request type
#define bmREQ_SET           USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_STANDARD|USB_SETUP_RECIPIENT_DEVICE     //set request type for all but 'set feature' and 'set interface'
#define bmREQ_CL_GET_INTF   USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE     //get interface request type

#define bmREQ_HIDOUT        USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE
#define bmREQ_HIDIN         USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE

// D7           data transfer direction (0 - host-to-device, 1 - device-to-host)
// D6-5         Type (0- standard, 1 - class, 2 - vendor, 3 - reserved)
// D4-0         Recipient (0 - device, 1 - interface, 2 - endpoint, 3 - other, 4..31 - reserved)

// USB Device Classes
#define USB_CLASS_USE_CLASS_INFO        0x00    // Use Class Info in the Interface Descriptors
#define USB_CLASS_AUDIO                 0x01    // Audio
#define USB_CLASS_COM_AND_CDC_CTRL      0x02    // Communications and CDC Control
#define USB_CLASS_HID                   0x03    // HID
#define USB_CLASS_PHYSICAL              0x05    // Physical
#define USB_CLASS_IMAGE                 0x06    // Image
#define USB_CLASS_PRINTER               0x07    // Printer
#define USB_CLASS_MASS_STORAGE          0x08    // Mass Storage
#define USB_CLASS_HUB                   0x09    // Hub
#define USB_CLASS_CDC_DATA              0x0a    // CDC-Data
#define USB_CLASS_SMART_CARD            0x0b    // Smart-Card
#define USB_CLASS_CONTENT_SECURITY      0x0d    // Content Security
#define USB_CLASS_VIDEO                 0x0e    // Video
#define USB_CLASS_PERSONAL_HEALTH       0x0f    // Personal Healthcare
#define USB_CLASS_DIAGNOSTIC_DEVICE     0xdc    // Diagnostic Device
#define USB_CLASS_WIRELESS_CTRL         0xe0    // Wireless Controller
#define USB_CLASS_MISC                  0xef    // Miscellaneous
#define USB_CLASS_APP_SPECIFIC          0xfe    // Application Specific
#define USB_CLASS_VENDOR_SPECIFIC       0xff    // Vendor Specific

// Additional Error Codes
#define USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED       0xD1
#define USB_DEV_CONFIG_ERROR_DEVICE_INIT_INCOMPLETE     0xD2
#define USB_ERROR_UNABLE_TO_REGISTER_DEVICE_CLASS       0xD3
#define USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL          0xD4
#define USB_ERROR_HUB_ADDRESS_OVERFLOW                  0xD5
#define USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL             0xD6
#define USB_ERROR_EPINFO_IS_NULL                        0xD7
#define USB_ERROR_INVALID_ARGUMENT                      0xD8
#define USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE         0xD9
#define USB_ERROR_INVALID_MAX_PKT_SIZE                  0xDA
#define USB_ERROR_EP_NOT_FOUND_IN_TBL                   0xDB
#define USB_ERROR_CONFIG_REQUIRES_ADDITIONAL_RESET      0xE0
#define USB_ERROR_FailGetDevDescr                       0xE1
#define USB_ERROR_FailSetDevTblEntry                    0xE2
#define USB_ERROR_FailGetConfDescr                      0xE3
#define USB_ERROR_TRANSFER_TIMEOUT                      0xFF

#define USB_XFER_TIMEOUT        5000    // (5000) USB transfer timeout in milliseconds, per section 9.2.6.1 of USB 2.0 spec
#define USB_NAK_LIMIT         	32000   // NAK limit for a transfer. 0 means NAKs are not counted
#define USB_RETRY_LIMIT         3       // 3 retry limit for a transfer
#define USB_SETTLE_DELAY        200     // settle delay in milliseconds

#define HUB_MAX_HUBS          	1       // maximum number of hubs that can be attached to the host controller
#define USB_NUMDEVICES          7 + HUB_MAX_HUBS + 1    //number of USB devices, +1 for address 0 for initializing
#define USB_NUM_PORTS          	8		//number of USB devices not counting hubs or port zero which is used only for enumeration
#define HUB_PORT_RESET_DELAY    20      // hub port reset delay 10 ms recommended, can be up to 20 ms
#define MAX_NUM_EP				3

/* USB state machine states */
#define USB_STATE_MASK                                      0xf0

#define USB_STATE_DETACHED                                  0x10
#define USB_DETACHED_SUBSTATE_INITIALIZE                    0x11
#define USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE               0x12
#define USB_DETACHED_SUBSTATE_ILLEGAL                       0x13
#define USB_ATTACHED_SUBSTATE_SETTLE                        0x20
#define USB_ATTACHED_SUBSTATE_RESET_DEVICE                  0x30
#define USB_ATTACHED_SUBSTATE_WAIT_RESET_COMPLETE           0x40
#define USB_ATTACHED_SUBSTATE_WAIT_SOF                      0x50
#define USB_ATTACHED_SUBSTATE_WAIT_RESET                    0x51
#define USB_ATTACHED_SUBSTATE_GET_DEVICE_DESCRIPTOR_SIZE    0x60
#define USB_STATE_ADDRESSING                                0x70
#define USB_STATE_CONFIGURING                               0x80
#define USB_STATE_RUNNING                                   0x90
#define USB_STATE_ERROR                                     0xa0

/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/

#endif /* SRC_DRIVERS_USB_HOST_USBCORE_H_ */
