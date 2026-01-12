/*
 * SEGGER RTT (minimal implementation).
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SEGGER_RTT_H
#define SEGGER_RTT_H

#include <stdarg.h>
#include <stdint.h>
#include "SEGGER_RTT_Conf.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *sName;
    char *pBuffer;
    uint32_t SizeOfBuffer;
    volatile uint32_t WrOff;
    volatile uint32_t RdOff;
    uint32_t Flags;
} SEGGER_RTT_BUFFER_UP;

typedef struct {
    const char *sName;
    char *pBuffer;
    uint32_t SizeOfBuffer;
    volatile uint32_t WrOff;
    volatile uint32_t RdOff;
    uint32_t Flags;
} SEGGER_RTT_BUFFER_DOWN;

typedef struct {
    char acID[16];
    int32_t MaxNumUpBuffers;
    int32_t MaxNumDownBuffers;
    SEGGER_RTT_BUFFER_UP aUp[SEGGER_RTT_MAX_NUM_UP_BUFFERS];
    SEGGER_RTT_BUFFER_DOWN aDown[SEGGER_RTT_MAX_NUM_DOWN_BUFFERS];
} SEGGER_RTT_CB;

void SEGGER_RTT_Init(void);
unsigned SEGGER_RTT_Write(unsigned BufferIndex, const void *pBuffer, unsigned NumBytes);
int SEGGER_RTT_printf(unsigned BufferIndex, const char *sFormat, ...);

#ifdef __cplusplus
}
#endif

#endif /* SEGGER_RTT_H */
