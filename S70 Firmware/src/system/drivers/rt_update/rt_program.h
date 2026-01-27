#ifndef SRC_SYSTEM_DRIVERS_RT_UPDATE_RT_PROGRAM_H_
#define SRC_SYSTEM_DRIVERS_RT_UPDATE_RT_PROGRAM_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "usb_slave.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Flags the USB CDC stream to be treated as Intel HEX for RT update.
 */
extern bool load_rt_hex;

/**
 * @brief Begin a firmware update session for the RT1064 target.
 */
void rt_firmware_update_start(USB_Slave_Message *slave_message);

/**
 * @brief Consume raw bytes from the USB CDC stream while updating.
 *
 * @return Number of bytes consumed from the input buffer.
 */
size_t rt_firmware_update_consume(const uint8_t *buffer, size_t length,
                                  USB_Slave_Message *slave_message);

/**
 * @brief Abort the firmware update session and reset internal state.
 */
void rt_firmware_update_abort(USB_Slave_Message *slave_message);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_RT_UPDATE_RT_PROGRAM_H_ */
