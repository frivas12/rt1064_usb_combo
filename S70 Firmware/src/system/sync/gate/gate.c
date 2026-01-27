//gate.c

#include "gate.h"
#include "semphr.h"

/*****************************************************************************
 * Data Types
 *****************************************************************************/



/*****************************************************************************
 * Constants
 *****************************************************************************/



/*****************************************************************************
 * Macros
 *****************************************************************************/



/*****************************************************************************
 * Static Data
 *****************************************************************************/



/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/



/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/



/*****************************************************************************
 * Public Functions
 *****************************************************************************/

GateHandle_t xGateCreate(void)
{
    return (GateHandle_t)xQueueCreate(1, 0);
}

#if configSUPPORT_STATIC_ALLOCATION
GateHandle_t xGateCreateStatic(StaticGate_t * const xpGateBuffer)
{
    return (GateHandle_t)xQueueCreateStatic((StaticQueue_t * const)xpGateBuffer);
}
#endif


void vGateDelete(GateHandle_t xGate)
{
    vQueueDelete((QueueHandle_t)xGate);
}


BaseType_t xGateOpen(GateHandle_t xGate)
{
    return xQueueOverwrite((QueueHandle_t) xGate, NULL);
}


BaseType_t xGateOpenFromISR(GateHandle_t xGate, BaseType_t * pxHigherPriorityTaskWoken)
{
    return xQueueOverwriteFromISR((QueueHandle_t)xGate, NULL, pxHigherPriorityTaskWoken);
}


BaseType_t xGateClose(GateHandle_t xGate)
{
    return xQueueReset((QueueHandle_t) xGate);
}


BaseType_t xGateCloseFromISR(GateHandle_t xGate, BaseType_t * pxHigherPriorityTaskWoken)
{
    return xQueueReceiveFromISR((QueueHandle_t) xGate, NULL, pxHigherPriorityTaskWoken);
}

BaseType_t xGatePass(GateHandle_t xGate, TickType_t xTicksToWait)
{
    BaseType_t rt = xQueueReceive((QueueHandle_t)xGate, NULL, xTicksToWait);
    if (rt == pdPASS) {
        xQueueOverwrite((QueueHandle_t) xGate, NULL);
    }
    return rt;
}

BaseType_t xGatePassFromISR(GateHandle_t xGate, BaseType_t * pxHigherPriorityTaskWoken)
{
    BaseType_t rt = xQueueReceiveFromISR((QueueHandle_t)xGate, NULL, pxHigherPriorityTaskWoken);
    if (rt == pdPASS) {
        xQueueOverwriteFromISR((QueueHandle_t)xGate, NULL, pxHigherPriorityTaskWoken);
    }
    return rt;
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/



//EOF
