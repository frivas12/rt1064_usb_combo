/**
 * @file sys_task.c
 *
 * @brief Functions for ???
 *
 */

#include <asf.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

TaskHandle_t xDeviceHandle[USB_NUMDEVICES] = { NULL };
TaskHandle_t xSlotHandle[NUMBER_OF_BOARD_SLOTS] = { NULL };
TaskHandle_t xSupervisorHandle = NULL;
TaskHandle_t xStepperSMHandle = NULL;
TaskHandle_t xDeviceDetection = NULL;
// QueueHandle_t xStepper_SM_Rx_Queue[NUMBER_OF_BOARD_SLOTS] = { NULL };
// QueueHandle_t xStepper_SM_Tx_Queue[NUMBER_OF_BOARD_SLOTS] = { NULL };
// QueueHandle_t xPiezo_SM_Rx_Queue[NUMBER_OF_BOARD_SLOTS] = { NULL };
// QueueHandle_t xPiezo_SM_Tx_Queue[NUMBER_OF_BOARD_SLOTS] = { NULL };
QueueHandle_t xUSB_Slave_Rx_Queue[NUMBER_OF_BOARD_SLOTS+1] = { NULL };
QueueHandle_t xFTDI_USB_Slave_Rx_Queue[NUMBER_OF_BOARD_SLOTS+1] = { NULL };

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void suspend_all_task(void)
{
	/* TODO STOP ALL flag*/

	/* Suspend all the Host USB task*/
	for (uint8_t i = 1; i < USB_NUMDEVICES; ++i)
	{
		if (xDeviceHandle[i] != NULL)
		{
			vTaskSuspend(xDeviceHandle[i]);
		}
	}

	/* Suspend all the Slot task*/
	for (uint8_t i = 0; i < NUMBER_OF_BOARD_SLOTS; ++i)
	{
		if (xSlotHandle[i] != NULL)
		{
			vTaskSuspend(xSlotHandle[i]);
		}
	}

	if (xSupervisorHandle != NULL)
	{
		vTaskSuspend(xSupervisorHandle);
	}

	if (xStepperSMHandle != NULL)
	{
		vTaskSuspend(xStepperSMHandle);
	}

}

void resume_all_task(void)
{
	/* TODO STOP ALL flag or pick and choose*/

	/* Resume all the USB task*/
	for (uint8_t i = 1; i < USB_NUMDEVICES; ++i)
	{
		if (xDeviceHandle[i] != NULL)
		{
			vTaskResume(xDeviceHandle[i]);
		}
	}

	/* Suspend all the Slot task*/
	for (uint8_t i = 0; i < NUMBER_OF_BOARD_SLOTS; ++i)
	{
		if (xSlotHandle[i] != NULL)
		{
			vTaskResume(xSlotHandle[i]);
		}
	}

	if (xSupervisorHandle != NULL)
	{
		vTaskResume(xSupervisorHandle);
	}

	if (xStepperSMHandle != NULL)
	{
		vTaskResume(xStepperSMHandle);
	}

}

