/**
 * \file
 *
 * \brief User board configuration template
 *
 */
/*
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */

#ifndef CONF_BOARD_H
#define CONF_BOARD_H

/*Stepper*/
//#define ENABLE_SYSTEM_LOGGING (1)
#ifndef ENABLE_SYSTEM_LOGGING
#define ENABLE_SYSTEM_LOGGING (0)
#endif

#define RELEASE_ENABLE (0)

#if RELEASE_ENABLE
#define DEBUG_ENABLE (0)
#define DEBUG_ERRORS (0)
#define DEBUG_USB_FTDI (0)
#define DEBUG_USB_HOST (0)
#define DEBUG_HID_REPORT (0)
#define DEBUG_USB_HOST_JOYSTICK (0)
#define DEBUG_DEVICE_DETECT (0)
#define ENABLE_STEPPER_LOG_STATUS_FLAGS (0)             // When enabled, the stepper log will include the status
                                                        // bits of the chip.

#define DEBUG_PID_OZONE (0)

#define SYSTEM_VIEW_ENABLE (0)

#define CONF_BOARD_USB_PORT
#define CONF_BOARD_ENABLE_CACHE_AT_INIT
//#define CONF_BOARD_CONFIG_MPU_AT_INIT

#else
/**
 * To disable all debugging for release set DEBUG_ENABLE to 0 and SYSTEM_VIEW_ENABLE to 0.
 * To turn on debug printing to serial terminal port, set both to 1.
 */

/* setting to zero disables all debug printing*/
#define DEBUG_ENABLE (1)
#define DEBUG_ERRORS (1)
#define DEBUG_USB_FTDI (0)
#define DEBUG_USB_HOST (0)
#define DEBUG_HID_REPORT (0)
#define DEBUG_USB_HOST_JOYSTICK (0)
#define DEBUG_DEVICE_DETECT (0)

#define DEBUG_PID_OZONE (0)
#define DEBUG_PID_JSCOPE (0)

#define SYSTEM_VIEW_ENABLE (1)

#define CONF_BOARD_USB_PORT
#define CONF_BOARD_ENABLE_CACHE_AT_INIT
//#define CONF_BOARD_CONFIG_MPU_AT_INIT
#endif
#endif // CONF_BOARD_H
