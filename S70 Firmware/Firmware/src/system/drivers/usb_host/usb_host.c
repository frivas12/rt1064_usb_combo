/**
 * @file usb_host
 *
 * @brief Functions for Initialize the MAX3421e chip and create the USB host task.
 * Also responsible for checking for connections/disconnections	from the root USB port
 * and hub ports if any.  When a new connection is detected, this task will add another
 * task for the device connected to the root port. If a device is disconnected, it will
 * delete all the task created here or by the HUB task
 *
 */

#include "usb_host.h"
#include "Debugging.h"
#include <asf.h>
#include <pins.h>
#include "max3421e.h"
#include "UsbCore.h"
#include "usb_device.h"
#include "usb_hub.h"
#include "hid.h"
#include "string.h"

extern SemaphoreHandle_t xSPI_Semaphore;

/****************************************************************************
 * Private Data
 ****************************************************************************/
USB_Host_root host_root;

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static void remove_root_device(void);
static void _do_task(void);
static void task_usb_host(void *pvParameters);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void remove_root_device(void)
{
	 /*If its a hub, we need to remove all the devices connected to it.*/
	if (usb_device[HUB_ADDRESS].is_hub)
	{
		debug_print("\n** USB HUB disconnected **\n");
		for (uint8_t address = USB_NUMDEVICES; address > 0; address--)
		{
			if (usb_device[address].enumerated)
				remove_usb_device(address);
		}
	}
	else
	{
		remove_usb_device(1); // only one device connect
		usb_host_print("\n** USB device disconnected **\n");
	}

}

/**
 * @brief This function will perform actions when a device is connected or disconnected from the
 * 			root port only.  If a device is connected, it will create a new task for the device.
 * 			The task will then try to enumerate the device.  If a device is disconnected, this will
 * 			remove that device.
 */
static void _do_task(void)
{
	uint8_t tmpdata;
	UsbNewDevice new_device;

	switch (host_root.bus_state)
	{
	case SE0: /* USB device disconnected*/
		if ((host_root.usb_task_state & USB_STATE_MASK) != USB_STATE_DETACHED)
			host_root.usb_task_state = USB_DETACHED_SUBSTATE_INITIALIZE;
		break;

	case SE1: /* illegal state */
		host_root.usb_task_state = USB_DETACHED_SUBSTATE_ILLEGAL;
		break;

	case LSHOST: /* low speed device connected */
		host_root.lowspeed = true;
		goto set_task;
		break;

	case FSHOST: /* full speed device connected */
		host_root.lowspeed = false;
		set_task: if ((host_root.usb_task_state & USB_STATE_MASK)
				== USB_STATE_DETACHED)
		{
			host_root.usb_task_state = USB_ATTACHED_SUBSTATE_SETTLE;
			return;
		}
		break;

	case NO_CHANGE: /* no interrupt detect so return no change */
		break;
	}

	switch (host_root.usb_task_state)
	{
	case USB_DETACHED_SUBSTATE_INITIALIZE:
		remove_root_device();
		host_root.usb_task_state = USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE;
		break;

	case USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE:
		/* just sit here until connection state changes*/
		break;

	case USB_DETACHED_SUBSTATE_ILLEGAL:
		/* just sit here until connection state changes*/
		break;

	case USB_ATTACHED_SUBSTATE_SETTLE: /* settle time for just attached device*/
		vTaskDelay(pdMS_TO_TICKS(200));
		host_root.usb_task_state = USB_ATTACHED_SUBSTATE_RESET_DEVICE;
		break;

	case USB_ATTACHED_SUBSTATE_RESET_DEVICE:
		xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
		max3421e_write(rHCTL, bmBUSRST); /* issue bus reset*/
		xSemaphoreGive(xSPI_Semaphore);
		host_root.usb_task_state = USB_ATTACHED_SUBSTATE_WAIT_RESET_COMPLETE;
		break;

	case USB_ATTACHED_SUBSTATE_WAIT_RESET_COMPLETE:
		xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
		if ((max3421e_read(rHCTL) & bmBUSRST) == 0)
		{
			/* start SOF generation*/
			tmpdata = max3421e_read(rMODE) | bmSOFKAENAB;
			max3421e_write(rMODE, tmpdata);
			host_root.usb_task_state = USB_ATTACHED_SUBSTATE_WAIT_SOF;
		}
		xSemaphoreGive(xSPI_Semaphore);
		break;

	case USB_ATTACHED_SUBSTATE_WAIT_SOF:
	 	xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
		if (max3421e_read(rHIRQ) & bmFRAMEIRQ)
		{
			xSemaphoreGive(xSPI_Semaphore);
			vTaskDelay(pdMS_TO_TICKS(200));
			host_root.usb_task_state = USB_ATTACHED_SUBSTATE_WAIT_RESET;
		}
		else
		{
			xSemaphoreGive(xSPI_Semaphore);
		}
		break;

	case USB_ATTACHED_SUBSTATE_WAIT_RESET:
		/*  Wait some frames before programming any transfers. This allows the device to
		 *  recover from the bus reset.
		 */
		vTaskDelay(pdMS_TO_TICKS(20));
		host_root.usb_task_state = USB_STATE_CONFIGURING;
		break; // don't fall through

	case USB_STATE_CONFIGURING:
		new_device.address = 1;
		new_device.lowspeed = host_root.lowspeed;

		/* Add a new task for the device connected to the root port */
		usb_device_add_device_task(&new_device);
		host_root.usb_task_state = USB_STATE_RUNNING;
		break;

	case USB_STATE_RUNNING:
		/* just sit here until connection state changes*/
		break;
	}
}

/****************************************************************************
 * Task
 ****************************************************************************/
/**
 * @brief This is the USB host task responsible for checking for connections/disconnections
 * 			from the root USB port and hub ports if any.  When a new connection is detected,
 * 			this task will add another task for the device connected to the root port. If a
 * 			device is disconnected, it will delete all the task created here or by the HUB task.
 *
 * @param pvParameters	: Not used.
 */
static void task_usb_host(void *pvParameters)
{
	host_root.usb_task_state = USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE;
	UNUSED(pvParameters);
	vTaskDelay(pdMS_TO_TICKS(500));
	for (;;)
	{
		/* Poll the interrupt pin to see if we had a connection change to the root port*/
		host_root.bus_state = MAX3421E_Task();

		/*If we had a connection change on the root port, do all the task to either create
		 * or destroy the new device*/
		_do_task();

		/* check hub status for connection changes on any of it's ports*/
		xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
		Poll_hub();
		xSemaphoreGive(xSPI_Semaphore);
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/**
 * @brief Removes a USB device.  This function will reset the USB structure,
 * 			 free any allocated memory, and delete the device task.
 *
 * @param address : address of device to remove.
 */
void remove_usb_device(uint8_t address)
{
	em_stop(EM_STOP_ALL);	// send stop signal to all slots

	TaskHandle_t temp_handle = xDeviceHandle[address];

	/* Reset the structure*/
	memset(&usb_device[address], 0, sizeof(UsbDevice));

	/* free structure for this task.*/
	if (pDev[address] != NULL)
	{
		vPortFree(pDev[address]);
		pDev[address] = NULL;
	}

	// Use the handle to delete the task.
	if (xDeviceHandle[address] != NULL)
	{
		xDeviceHandle[address] = NULL;
		vTaskDelete(temp_handle);
	}
}

/**
 * @brief Initialize the MAX3421e chip and create the USB host task.
 *
 *
 */
// MARK:  SPI Mutex Required
void usb_host_init(void)
{
	static bool ready = 0;

    // Mappings are handled by the hid_mapping_service.

	/*Reset all USB device structures */
	for (int i = 1; i < USB_NUMDEVICES; ++i)
	{
		memset(&usb_device[i], 0, sizeof(UsbDevice));
	}

	ready = !max3421e_init();

	if (ready)
	{
		/* Create task to for the USB host */
		if (xTaskCreate(task_usb_host, "USB Host", TASK_USB_HOST_STACK_SIZE,
		NULL,
		TASK_USB_HOST_STACK_PRIORITY, &xDeviceHandle[0]) != pdPASS)
		{
			error_print("ERROR: Failed to create USB Host task\r\n");
		}
	}
}

