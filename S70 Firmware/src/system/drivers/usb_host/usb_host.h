// usb_host.h

#ifndef SRC_SYSTEM_DRIVERS_USB_HOST_USB_HOST_H_
#define SRC_SYSTEM_DRIVERS_USB_HOST_USB_HOST_H_

#include "compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "usb_ch9.h"

/****************************************************************************
 * Defines
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void remove_usb_device(uint8_t address);
void usb_host_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_USB_HOST_USB_HOST_H_ */
