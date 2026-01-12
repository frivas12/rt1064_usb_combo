/*
 * SEGGER RTT (minimal implementation).
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "SEGGER_RTT.h"

#define RTT_MAGIC "SEGGER RTT"

static SEGGER_RTT_CB _SEGGER_RTT;
static char _SEGGER_RTT_UpBuffer[SEGGER_RTT_BUFFER_SIZE_UP];
static char _SEGGER_RTT_DownBuffer[SEGGER_RTT_BUFFER_SIZE_DOWN];

static uint32_t SEGGER_RTT_GetAvailWriteSpace(const SEGGER_RTT_BUFFER_UP *pRing)
{
    uint32_t rdOff = pRing->RdOff;
    uint32_t wrOff = pRing->WrOff;

    if (wrOff >= rdOff) {
        return (pRing->SizeOfBuffer - 1U) - (wrOff - rdOff);
    }
    return (rdOff - wrOff - 1U);
}

void SEGGER_RTT_Init(void)
{
    SEGGER_RTT_LOCK();
    if (_SEGGER_RTT.acID[0] == '\0') {
        uint32_t idx;

        for (idx = 0; idx < sizeof(_SEGGER_RTT.acID); idx++) {
            _SEGGER_RTT.acID[idx] = RTT_MAGIC[idx];
            if (RTT_MAGIC[idx] == '\0') {
                break;
            }
        }

        _SEGGER_RTT.MaxNumUpBuffers = (int32_t)SEGGER_RTT_MAX_NUM_UP_BUFFERS;
        _SEGGER_RTT.MaxNumDownBuffers = (int32_t)SEGGER_RTT_MAX_NUM_DOWN_BUFFERS;

        _SEGGER_RTT.aUp[0].sName = "Terminal";
        _SEGGER_RTT.aUp[0].pBuffer = _SEGGER_RTT_UpBuffer;
        _SEGGER_RTT.aUp[0].SizeOfBuffer = SEGGER_RTT_BUFFER_SIZE_UP;
        _SEGGER_RTT.aUp[0].WrOff = 0U;
        _SEGGER_RTT.aUp[0].RdOff = 0U;
        _SEGGER_RTT.aUp[0].Flags = SEGGER_RTT_MODE_DEFAULT;

        _SEGGER_RTT.aDown[0].sName = "Terminal";
        _SEGGER_RTT.aDown[0].pBuffer = _SEGGER_RTT_DownBuffer;
        _SEGGER_RTT.aDown[0].SizeOfBuffer = SEGGER_RTT_BUFFER_SIZE_DOWN;
        _SEGGER_RTT.aDown[0].WrOff = 0U;
        _SEGGER_RTT.aDown[0].RdOff = 0U;
        _SEGGER_RTT.aDown[0].Flags = SEGGER_RTT_MODE_DEFAULT;
    }
    SEGGER_RTT_UNLOCK();
}

unsigned SEGGER_RTT_Write(unsigned BufferIndex, const void *pBuffer, unsigned NumBytes)
{
    const uint8_t *pData = (const uint8_t *)pBuffer;
    unsigned bytesWritten = 0U;

    if (BufferIndex >= SEGGER_RTT_MAX_NUM_UP_BUFFERS) {
        return 0U;
    }

    if (_SEGGER_RTT.acID[0] == '\0') {
        SEGGER_RTT_Init();
    }

    SEGGER_RTT_LOCK();
    SEGGER_RTT_BUFFER_UP *pRing = &_SEGGER_RTT.aUp[BufferIndex];
    uint32_t avail = SEGGER_RTT_GetAvailWriteSpace(pRing);

    if (avail == 0U && pRing->Flags == SEGGER_RTT_MODE_NO_BLOCK_SKIP) {
        SEGGER_RTT_UNLOCK();
        return 0U;
    }

    if (avail < NumBytes && pRing->Flags == SEGGER_RTT_MODE_NO_BLOCK_TRIM) {
        NumBytes = (unsigned)avail;
    }

    while (bytesWritten < NumBytes) {
        while (SEGGER_RTT_GetAvailWriteSpace(pRing) == 0U) {
            if (pRing->Flags != SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL) {
                SEGGER_RTT_UNLOCK();
                return bytesWritten;
            }
        }

        pRing->pBuffer[pRing->WrOff] = (char)pData[bytesWritten];
        pRing->WrOff++;
        if (pRing->WrOff >= pRing->SizeOfBuffer) {
            pRing->WrOff = 0U;
        }
        bytesWritten++;
    }

    SEGGER_RTT_UNLOCK();
    return bytesWritten;
}
