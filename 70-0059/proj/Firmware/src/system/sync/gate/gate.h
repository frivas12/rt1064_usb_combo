//gate.h

/**************************************************************************//**
 * \file gate.h
 * \author Sean Benish
 * \brief Implements the gate synchronization object.
 *
 * A gate is a synchronization structure that allows one thread to control
 * the run time of another thread at a specific section of code.  This can
 * be used in looping threads to block until another thread has opened the gate.
 *****************************************************************************/

#ifndef SRC_SYSTEM_SYNC_GATE_GATE_H_
#define SRC_SYSTEM_SYNC_GATE_GATE_H_

#include "FreeRTOS.h"
#include "queue.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*****************************************************************************
 * Defines
 *****************************************************************************/



/*****************************************************************************
 * Data Types
 *****************************************************************************/

typedef QueueHandle_t GateHandle_t;
typedef StaticQueue_t StaticGate_t;

/*****************************************************************************
 * Constants
 *****************************************************************************/



/*****************************************************************************
 * Public Functions
 *****************************************************************************/

// Documenting style has been copied from RTOS's semaphore documentation.

/**
 * Creates a gate, and returns a handle by which the gate can be referenced.
 *
 * Each gate requires a small amount of RAM that is used to hold the gate’s
 * state. If a gate is created using xGateCreate() then the required
 * RAM is automatically allocated from the FreeRTOS heap. If a gate is created
 * using xGateCreateStatic() then the RAM is provided by the application writer,
 * which requires an additional parameter, but allows the RAM to be statically allocated at
 * compile time.
 *
 * The gate is created in the 'closed' state, meaning the gate must first be
 * opened before it can be passed using the xGatePass() function.
 * \return NULL
 *         Any other value
 */
GateHandle_t xGateCreate(void);

#if configSUPPORT_STATIC_ALLOCATION
/**
 * Creates a gate, and returns a handle by which the gate can be referenced.
 *
 * Each gate requires a small amount of RAM that is used to hold the gate’s
 * state. If a gate is created using xGateCreate() then the required
 * RAM is automatically allocated from the FreeRTOS heap. If a gate is created
 * using xGateCreateStatic() then the RAM is provided by the application writer,
 * which requires an additional parameter, but allows the RAM to be statically allocated at
 * compile time.
 *
 * The gate is created in the 'closed' state, meaning the gate must first be
 * opened before it can be passed using the xGatePass() function.
 * \param xpGateBuffer
 * \return
 */
GateHandle_t xGateCreateStatic(StaticGate_t * const xpGateBuffer);
#endif

/**
 * Deletes a gate that was previously created using a call to xGateCreate().
 * \param xGate The handle of the gate being deleted.
 * \note Tasks can opt to block on a gate if they attempt to obtain a gate that is not available.
 * A gate must \em not be deleted if there are any tasks currently blocked on it.
 */
void vGateDelete(GateHandle_t xGate);

/**
 * "Opens" a gate that has previously been created using a call to
 * xGateCreate or xGateCreateStatic.
 * \param xGate The gate being "openned".
 * \return BaseType_t   pdPASS  Returned if the call successfully openned the gate.
 *                      pdFAIL  Returned if the call did not successfully openned the gate.
 */
BaseType_t xGateOpen(GateHandle_t xGate);

/**
 * A version of xGateOpen() that can be called from an ISR.
 * \param xGate The gate being "openned".
 * \param pxHigherPriorityTaskWoken Sets the value to pdTRUE if a context switch should be performed
 * before the interrupt is exited; pdFALSE otherwise.  Optional (set to NULL).
 * \return BaseType_t   pdPASS  Returned if the call successfully openned the gate.
 *                      pdFAIL  Returned if the call did not open the gate.
 */
BaseType_t xGateOpenFromISR(GateHandle_t xGate, BaseType_t * pxHigherPriorityTaskWoken);

/**
 * "Closes" a gate that has previously been created using a call to
 * xGateCreate or xGateCreateStatic.
 * \param xGate The gate being "closed".
 * \return BaseType_t   pdPASS  Returned if the call succesfully closed the gate.
 *                      pdFAIL  Returned if the call did not close the gate.
 */
BaseType_t xGateClose(GateHandle_t xGate);

/**
 * A version of xGateClose() that can be called from an ISR.
 * \param xGate The gate being "closed".
 * \param pxHigherPriorityTaskWoken Sets the value to pdTRUE if a context switch should be performed
 * before the interrupt is exited; pdFALSE otherwise.  Optional (set to NULL).
 * \return BaseType_t   pdPASS  Returned if the call succesfully closed the gate.
 *                      pdFAIL  Returned if the call did not close the gate.
 */
BaseType_t xGateCloseFromISR(GateHandle_t xGate, BaseType_t * pxHigherPriorityTaskWoken);

/**
 * Causes the thread to wait until either xTicksToWait is up, or the gate is openned.
 * If the gate is closed, it blocks the thread.
 * \param xGate The gate to pass through.
 * \param xTicksToWait The maximum amount of time to wait.  If set to 0, it can be
 * used to poll the status of the gate.
 * \return BaseType_t   pdPASS  The gate was passed through.
 *                      pdFAIL  The gate was not passed through, most likely because 
 *                              the timeout of xTicksToWait was reached.
 */
BaseType_t xGatePass(GateHandle_t xGate, TickType_t xTicksToWait);

/**
 * A version of xGatePass() that can be called from an ISR.
 * \param xGate The gate to pass through.
 * \param pxHigherPriorityTaskWoken Sets the value to pdTRUE if a context switch should be performed
 * before the interrupt is exited; pdFALSE otherwise.  Optional (set to NULL).
 * \return BaseType_t   pdPASS  The gate was passed through.
 *                      pdFAIL  The gate was not passed through, most likely because 
 *                              the timeout of ticks_to_wait was reached.
 */
BaseType_t xGatePassFromISR(GateHandle_t xGate, BaseType_t * pxHigherPriorityTaskWoken);


#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_SYNC_GATE_GATE_H_ */
//EOF
