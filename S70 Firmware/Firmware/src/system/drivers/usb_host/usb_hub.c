/**
 * @file usb_hub.c
 *
 * @brief Functions for Hub connected to root port.  Only one hub is supported.
 *
 */

#include "Debugging.h"
#include <asf.h>
#include "max3421e.h"
#include "max3421e_usb.h"
#include "UsbCore.h"
#include "usb_hub.h"
#include "usb_device.h"
#include "usb_host.h"
#include "user_spi.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static uint8_t hub_port_feature(uint8_t address, uint8_t request, uint16_t feature);
static uint8_t get_hub_port_status(uint8_t address, HubEvent* buf);
static uint8_t check_hub_status(void);
static uint8_t hub_port_status_change(uint8_t address, HubEvent* evt);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/**
 * @brief Set or clear a hub port feature.
 * @param address : The port on the hub.
 * @param request : Type of request.
 * @param feature: Feature request.
 * @return : 0 if everything went well, else error
 */
// MARK:  SPI Mutex Required
static uint8_t hub_port_feature(uint8_t address, uint8_t request, uint16_t feature)
{
	uint8_t rcode = 0;

	uint8_t SetHubFeature[] =
	{		 0x23,        	// bmRequestType:
			request,        // bRequest: SET_FEATURE
			0x08, 0x00,     // wValue: feature
			0x02, 0x00,  	// wIndex: port (first port is 1); hub itself is port 0
			0x00, 0x00 }; 	// wLength: zero

	SetHubFeature[2] = feature & 0xFF;
	SetHubFeature[3] = (feature >> 8) & 0xFF;
	SetHubFeature[4] = address & 0xFF;
	SetHubFeature[5] = (address >> 8) & 0xFF;

	rcode = control_write(SetHubFeature);

	if (rcode)
	{
		error_print("ERROR: Hub port feature");
	}

	return rcode;
}

/**
 * @brief Get the status of a port on the hub.  We are only checking connection chnage but
 * 			we could also check things like over current.
 * @param address : The port on the hub.
 * @param buf : buffer to hold the data for the status.
  * @return : 0 if everything went well, else error
*/
// MARK:  SPI Mutex Required
static uint8_t get_hub_port_status(uint8_t address, HubEvent* buf)
{
	uint8_t rcode = 0;

	// Get_HUB_port_status A3 0 0 0 address 0 4 0
	uint8_t Get_HUB_port_status[8] =
	{ bmREQ_GET_PORT_STATUS, USB_REQUEST_GET_STATUS, 0x00, 0x00, address, 0x00, 4,
			0x00 };

	rcode = control_read(1, Get_HUB_port_status, (uint8_t*) buf);
	if (rcode)
	{
		error_print("ERROR: Can't get HUB port status\n");
	}

	return rcode;
}

/**
 * @brief Checks hub status.  This function also checks the status of all the ports on the hub.
 * 			Each time this function is called it checks the next port on the hub until it reaches
 * 			the last one where it will start over on the first port.
  * @return : 0 if everything went well, else error
 */
// MARK:  SPI Mutex Required
static uint8_t check_hub_status(void)
{
	uint8_t rcode = hrSUCCESS;
	uint8_t buf[8];
	uint8_t bytes_read = 0;
	uint8_t * ptr = &bytes_read;
	static uint8_t address = 1, mask = 0x02;;


	/*Read the status from the hub*/
	rcode = usb_read(HUB_ADDRESS, 1, 8, buf, ptr, 0);
	if (rcode)
	{
//		error_print("ERROR: Could not read hub status");
		return rcode;
	}

	if (buf[0] & mask) ///> How does this work?  Check the MAX chip.
	{
		HubEvent evt;
		evt.bmEvent = 0;

		/* Check the hub status*/
		rcode = get_hub_port_status(address, &evt);

		if (rcode)
			return rcode;

		/* Check the port status*/
		rcode = hub_port_status_change(address, &evt);
		if (rcode == HUB_ERROR_PORT_HAS_BEEN_RESET)
		return 0;

		if (rcode)
		return rcode;
	}

	/*increment for next time*/
	if (address < usb_device[HUB_ADDRESS].hub.bNumberOfPorts + 1)
	{
		address++;
		mask <<= 1;
	}
	else
	{
		address = 1;
		mask = 0x02;
	}
	return rcode;
}

/**
 * @brief Checks hub status.  This function also checks the status of all the ports on the hub.
 * 			Each time this function is called it checks the next port on the hub until it reaches
 * 			the last one where it will start over on the first port.
 * @param address :
 * @param evt
 * @return
 */
// MARK:  SPI Mutex Required
uint8_t hub_port_status_change(uint8_t address, HubEvent* evt)
{
	uint8_t rcode = hrSUCCESS;
	UsbNewDevice new_device;

	switch (evt->bmEvent)
	{
	/* High speed device connected event*/
	case bmHUB_PORT_EVENT_CONNECT:
		usb_device[address + 1].lowspeed = false;
		debug_print("Full speed device connected on port %d\n", address);
		goto next;

		/* Low speed device connected event*/
	case bmHUB_PORT_EVENT_LS_CONNECT:
		debug_print("Low speed device connected on port %d\n", address);
		usb_device[address + 1].lowspeed = true;
		next: if (usb_device[address + 1].hub.bResetInitiated)
			return 0;

		rcode = hub_port_feature(address, USB_REQUEST_CLEAR_FEATURE, HUB_FEATURE_C_PORT_ENABLE);
		rcode = hub_port_feature(address, USB_REQUEST_CLEAR_FEATURE, HUB_FEATURE_C_PORT_CONNECTION);
		rcode = hub_port_feature(address, USB_REQUEST_SET_FEATURE,	HUB_FEATURE_PORT_RESET);

		xSemaphoreGive(xSPI_Semaphore); /* Give back the SPI bus while we block*/
		vTaskDelay(pdMS_TO_TICKS(100));
		xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);

		usb_device[address + 1].hub.bResetInitiated = true;
		return HUB_ERROR_PORT_HAS_BEEN_RESET;

		/* Device disconnected event*/
	case bmHUB_PORT_EVENT_DISCONNECT:
		// TODO st_stop_all_steppers();
		rcode = hub_port_feature(address, USB_REQUEST_CLEAR_FEATURE, HUB_FEATURE_C_PORT_ENABLE);
		rcode = hub_port_feature(address, USB_REQUEST_CLEAR_FEATURE, HUB_FEATURE_C_PORT_CONNECTION);
		usb_device[HUB_ADDRESS].hub.bResetInitiated = false;
		remove_usb_device(address + 1);
		debug_print("Device removed on port %d\n", address);
		return 0;

		/* Reset complete event*/
	case bmHUB_PORT_EVENT_RESET_COMPLETE:
	case bmHUB_PORT_EVENT_LS_RESET_COMPLETE:
		rcode = hub_port_feature(address, USB_REQUEST_CLEAR_FEATURE, HUB_FEATURE_C_PORT_RESET);
		rcode = hub_port_feature(address, USB_REQUEST_CLEAR_FEATURE, HUB_FEATURE_C_PORT_CONNECTION);

		xSemaphoreGive(xSPI_Semaphore); /* Give back the SPI bus while we block*/
		vTaskDelay(pdMS_TO_TICKS(20));
		new_device.address = address + 1;
		new_device.lowspeed = usb_device[address + 1].lowspeed;
		usb_device[HUB_ADDRESS].hub.bResetInitiated = false;

		/* Add a new task for the device connected to the root port */
		usb_device_add_device_task(&new_device);

		xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
		break;
	}
	return rcode;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/**
 * @brief Get the hub descriptors to see how many port it has.
 * @param address : For now we only support one hub so the address should always
 * be 1.
 *
 * @return : 0 if everything went well, else error
 */
// MARK:  SPI Mutex Required
uint8_t GetHubDescriptor(uint8_t address)
{
	uint8_t rcode = 0;
	HubDescriptor buf;

	/* Get_Hub_Descriptor A0 06 00 29 00 00 9 00*/
	uint8_t Get_Hub_Descriptor[8] =
	{ bmREQ_GET_HUB_DESCRIPTOR, USB_REQUEST_GET_DESCRIPTOR, 0x00, 0x29, 0x00,
			0x00, 9, 0x00 };
	rcode = control_read(address, Get_Hub_Descriptor, (uint8_t*) &buf);
	if (rcode)
	{
		error_print("ERROR: Can't get USB HUB descriptor\n");
	}
	else
	{
		usb_device[address].hub.bNumberOfPorts = buf.bNumberOfPorts;
		usb_device[1].hub.PollEnable = false;
		usb_device[1].hub.bResetInitiated = false;
	}

	return rcode;
}

/**
 * @brief Power up all port on hub.
 *
 * @return : 0 if everything went well, else error
 */
// MARK:  SPI Mutex Required
uint8_t Hub_ports_pwr(void)
{
	uint8_t rcode = 0;

	for (uint8_t j = 1; j <= usb_device[HUB_ADDRESS].hub.bNumberOfPorts; j++)
	{
		rcode |= hub_port_feature(j, USB_REQUEST_SET_FEATURE,
		HUB_FEATURE_PORT_POWER);
	}
	return rcode;
}

/**
 * @brief Check to see if their is a hub on the root port.  If so and the PollEnable flag
 * 			is true, check the status of the ports on the hub for connect/disconnect.
 * 			HUB always address 1.
 * @return : 0 if everything went well, else error
 */
// MARK:  SPI Mutex Required
uint8_t Poll_hub(void)
{
	uint8_t rcode = 0;

	if (usb_device[HUB_ADDRESS].is_hub)
	{
		if (!usb_device[HUB_ADDRESS].hub.PollEnable)
			return 0;

		rcode = check_hub_status();
	}
	return rcode;
}
