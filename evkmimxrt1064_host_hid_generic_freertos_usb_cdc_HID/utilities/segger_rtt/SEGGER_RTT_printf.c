/*
 * SEGGER RTT printf helper (minimal implementation).
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "SEGGER_RTT.h"

#include <stdio.h>
#include <string.h>

int SEGGER_RTT_printf(unsigned BufferIndex, const char *sFormat, ...)
{
    char acBuffer[SEGGER_RTT_PRINTF_BUFFER_SIZE];
    va_list args;
    int length;

    va_start(args, sFormat);
    length = vsnprintf(acBuffer, sizeof(acBuffer), sFormat, args);
    va_end(args);

    if (length <= 0) {
        return length;
    }

    if ((size_t)length >= sizeof(acBuffer)) {
        length = (int)(sizeof(acBuffer) - 1U);
        acBuffer[length] = '\0';
    }

    (void)SEGGER_RTT_Write(BufferIndex, acBuffer, (unsigned)length);
    return length;
}
