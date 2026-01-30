// Used for code compatability
#pragma once

#include <cstdint>
#include <stdlib.h>

#define pvPortMalloc(a) malloc(a)
#define vPortFree(a) free(a)
#define portMAX_DELAY static_cast<TickType_t>(-1)
#define configASSERT(condition) while(!(condition)) {}

typedef uint32_t BaseType_t;
typedef BaseType_t TickType_t;


#define pdPASS static_cast<BaseType_t>(1)
#define pdFAIL static_cast<BaseType_t>(0)