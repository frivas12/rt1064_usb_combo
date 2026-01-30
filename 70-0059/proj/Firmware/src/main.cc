/**
 * @file
 *
 * @brief Main entry point for application
 *
 * ASF modifications
 * =================
 * - move the boards folder to system folder to keep it separate from the ASF
 * - added a way to tell what interrupt fired the Dummy_Handler in system_sams70.h
 *
 * Top level flow diagram
 * ======================
 *
 *@dot
 digraph G {
 node [shape=parallelogram]; "system_init()"; "main";
 node [shape=doublecircle]; "stepper n task"; "servo n task"; "usb dev task"; "usb host task"; "error task";
 node [shape=diamond]; "init_slots()";
 "main"->"system_init()"->"init_slots()";
 "init_slots()"->"stepper n task";
 "init_slots()"->"servo n task";
 "system_init()"-> "usb dev task";
 "system_init()"->"usb host task";
 "system_init()"->"error task";
 }
 * @enddot
 */

#include <asf.h>
#include <main.h>
#include "Debugging.h"

#include "FreeRTOS.h"

#ifdef USE_RTOS_DEBUGGING
#include "tc2.h"
#endif

/**
 * \brief Macro1
 */
#define TASK_MONITOR_STACK_SIZE            (2048/sizeof(portSTACK_TYPE))
/**
 * \brief Macro2
 */
#define TASK_MONITOR_STACK_PRIORITY        (tskIDLE_PRIORITY)
/**
 * \brief Macro3
 */

extern "C" void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName);
extern "C" void vApplicationIdleHook(void);
extern "C" void vApplicationTickHook(void);
extern "C" void vApplicationMallocFailedHook(void);
extern "C" void xPortSysTickHandler(void);

/**
 * \brief Called if stack overflow during execution
 */
extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName)
{
#if 1
	debug_print("ERROR: Stack overflow %x %s\r\n", pxTask, (portCHAR * ) pcTaskName);
	/* If the parameters have been corrupted then inspect pxCurrentTCB to
	 * identify which task has overflowed its stack.
	 */
	for (;;)
	{
	}
#endif
}

/**
 * \brief This function is called by FreeRTOS idle task
 */
extern "C" void vApplicationIdleHook(void)
{
	// Management of sleep mode in Idle Hook from FreeRTOS
}

/**
 * \brief This function is called by FreeRTOS each tick
 */
extern "C" void vApplicationTickHook(void)
{
//	ioport_toggle_pin_level(MY_LED);

}

extern "C" void vApplicationMallocFailedHook(void)
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	 free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	 internally by FreeRTOS API functions that create tasks, queues, software
	 timers, and semaphores.  The size of the FreeRTOS heap is set by the
	 configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

	/* Force an assert. */
	configASSERT(( volatile void * ) NULL);
}

/**
 * \brief This task, when activated, send every ten seconds on debug UART
 * the whole report of free heap and total tasks status
 */
// static void task_monitor(void *pvParameters)
// {
// #if 1
// 	static portCHAR szList[256];
// 	UNUSED(pvParameters);

// 	for (;;)
// 	{
// 		debug_print("--- task ## %u\n", (unsigned int )uxTaskGetNumberOfTasks());
// 		vTaskList((portCHAR *) szList);
// 		debug_print(szList);
// 		vTaskDelay(100);
// 	}
// #endif
// }

/**
 * \brief This task, when activated, make LED blink at a fixed rate
 */

/**
 *** APT notes ***
 *** -Need to add a command or integrate into MGMSG_HW_REQ_INFO to get the type of cards in the
 *** 	slots
 ***


 Things done since last git commit




 */

//Add timer ticks variable if using rtos debugging.
#ifdef USE_RTOS_DEBUGGING
extern "C" void vConfigureTimerForRunTimeStats(void)
{
	REG_TC2_CCR2 = TC_CCR_CLKDIS;
	REG_TC2_CMR2 = TC_CMR_TCCLKS_TIMER_CLOCK1 | TC_CMR_LDRA_RISING;


	REG_TC2_CCR2 = TC_CCR_CLKEN;
}
#endif


int main(void)
{
	system_init();

	/* Create task to monitor processor activity */
//	if (xTaskCreate(task_monitor, "Monitor", TASK_MONITOR_STACK_SIZE, NULL,
//			TASK_MONITOR_STACK_PRIORITY, NULL) != pdPASS)
//	{
//		debug_print("Failed to create Monitor task\r\n");
//	}
//

	/* Start the scheduler. */
	vTaskStartScheduler();

	/* Will only get here if there was insufficient memory to create the idle task. */
	return 0;
}

/**

 */
