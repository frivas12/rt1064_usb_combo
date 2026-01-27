/*
 * hid_pid_fixes.h
 *
 *  Created on: Jul 31, 2018
 *      Author: elieser
 */

#ifndef SRC_SYSTEM_DRIVERS_USB_HOST_HID_PID_FIXES_H_
#define SRC_SYSTEM_DRIVERS_USB_HOST_HID_PID_FIXES_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

void fix_hid_report_parsing_errors(uint8_t address);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_USB_HOST_HID_PID_FIXES_H_ */
