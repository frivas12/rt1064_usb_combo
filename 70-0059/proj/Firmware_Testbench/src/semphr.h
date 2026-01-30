// Used for code compatability
// Also provides testing code for checking semaphore issues.
#pragma once

#include "FreeRTOS.h"

typedef int* SemaphoreHandle_t;

bool xSemaphoreTake (SemaphoreHandle_t a, TickType_t b);
inline bool xSemaphoreTakeFromISR (SemaphoreHandle_t a, BaseType_t *)
{
    return xSemaphoreTake(a,0);
}
void xSemaphoreGive (SemaphoreHandle_t a);
inline void xSemaphoreGiveFromISR (SemaphoreHandle_t a, BaseType_t *)
{
    return xSemaphoreGive(a);
}
SemaphoreHandle_t xSemaphoreCreateMutex ();
void vSemaphoreDelete (SemaphoreHandle_t a);