/**
 * @file apt_parse.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_USB_SLAVE_APT_PARSE_H_
#define SRC_SYSTEM_DRIVERS_USB_SLAVE_APT_PARSE_H_

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
#define APT_EFS_TIMEOUT     pdMS_TO_TICKS(100)

/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
bool apt_parse(USB_Slave_Message *slave_message);
void usb_slave_command_not_supported(USB_Slave_Message *slave_message);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_USB_SLAVE_APT_PARSE_H_ */
