/*
 * SEGGER RTT configuration for the RT1064 USB combo project.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SEGGER_RTT_CONF_H
#define SEGGER_RTT_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#define SEGGER_RTT_MAX_NUM_UP_BUFFERS   (1U)
#define SEGGER_RTT_MAX_NUM_DOWN_BUFFERS (1U)

#define SEGGER_RTT_BUFFER_SIZE_UP   (1024U)
#define SEGGER_RTT_BUFFER_SIZE_DOWN (16U)

#define SEGGER_RTT_MODE_NO_BLOCK_SKIP        (0U)
#define SEGGER_RTT_MODE_NO_BLOCK_TRIM        (1U)
#define SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL   (2U)

#define SEGGER_RTT_MODE_DEFAULT SEGGER_RTT_MODE_NO_BLOCK_SKIP

#define SEGGER_RTT_PRINTF_BUFFER_SIZE (256U)

#define SEGGER_RTT_LOCK()
#define SEGGER_RTT_UNLOCK()

#ifdef __cplusplus
}
#endif

#endif /* SEGGER_RTT_CONF_H */
