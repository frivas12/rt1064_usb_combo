/**
 * @file max3421e_usb.c
 *
 * @brief Higher level functions for MAX3421E USB IC.
 *
 */

#include "Debugging.h"
#include <asf.h>
#include "usb_device.h"
#include "max3421e_usb.h"
#include "max3421e.h"
#include "hid_report.h"
#include "user_spi.h"
#include "string.h"
#include "hid.h"
#include "usb_hub.h"


/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static uint8_t set_configuration(uint8_t config);
static uint8_t set_protocol(uint8_t protocol, uint8_t interface);
static uint8_t dispatch_packet(uint8_t token, uint8_t endpoint, uint16_t nak_limit);

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/**
 * @brief Set the
 * @param config
 * @return
 */
// MARK:  SPI Mutex Required
static uint8_t set_configuration(uint8_t config)
{
	uint8_t rcode = 0;

	// 0x00, 0x09, config,
	uint8_t Set_Configuration[8] =
	{ bmREQ_SET, USB_REQUEST_SET_CONFIGURATION, config, 0x00, 0x00, 0x00, 0x00,
			0x00 };

	rcode = control_write(Set_Configuration);
	if (rcode)
	{
		error_print("EROR: Cannot set USB host configuration\r\n");
	}
	else
	{
		usb_host_print("Set configuration to %d\r\n", config);
	}
	return rcode;
}

// MARK:  SPI Mutex Required
static uint8_t set_protocol(uint8_t protocol, uint8_t interface)
{
	uint8_t rcode = 0;

	uint8_t Set_Proto[8] =
	{ bmREQ_HIDOUT, HID_REQUEST_SET_IDLE, protocol, 0x00, interface, 0x00, 0x00,
			0x00 };

	rcode = control_write(Set_Proto);
	if (rcode)
	{
		error_print("ERROR: Cannot set protocol and interface\r\n");
	}
	else
	{
		usb_host_print("Set protocol to %d with %d interface\r\n", protocol,
				interface);
	}
	return rcode;
}

/**
 * @brief The data is in the fifo, now we need to dispatch the packet to the device
 * @param token	: Host transfer token values for writing the HXFR register
 * @param endpoint: The endpoint we are using
 * @return 0 in case of success, error code otherwise
 */
// MARK:  SPI Mutex Required
static uint8_t dispatch_packet(uint8_t token, uint8_t endpoint, uint16_t nak_limit)
{
	uint8_t rcode = hrSUCCESS;
	uint8_t retry_count = 0;
	uint16_t nak_count = 0;
	TickType_t timeout = xTaskGetTickCount() + USB_XFER_TIMEOUT / 1;// ((float) 1000/configTICK_RATE_HZ);


	while (timeout > xTaskGetTickCount())
	{
		/* Launch the transfer.*/
		max3421e_write(rHXFR, (token | endpoint));

		/* Wait for interrupt*/
		while (timeout > xTaskGetTickCount())
		{
			if (max3421e_read(rHIRQ) & bmHXFRDNIRQ)
			{
				/* Clear the interrupt.*/
				max3421e_write(rHIRQ, bmHXFRDNIRQ);
				break;
			}
		}

		/* Wait for HRSL*/
		while (timeout > xTaskGetTickCount())
		{
			rcode = (max3421e_read(rHRSL) & 0x0f);
			if (rcode != hrBUSY)
				break;
			else
				usb_host_print("USB dispatch busy!\r\n");
		}

		switch (rcode)
		{
		case hrSUCCESS:
			return (rcode);
			break;

		case hrNAK:
			nak_count++;
			if (nak_count > nak_limit)
				return (rcode);
			break;

		case hrTIMEOUT:
			retry_count++;
			if (retry_count == USB_RETRY_LIMIT)
				return (rcode);
			break;

		default:
//			debug_print("USB dispatch error!\r\n");
			return (rcode);
			break;
		}
	}
	return (rcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/


/**
 * @brief Performs an in transfer from a USB device from an arbitrary endpoint.
 *
 * @param address : USB device address.
 * @param endpoint	: The enpoint we are using.
 * @param INbytes : how many bytes we are trying to read
 * @param buf	: The pointer to the buffer we will write to.
 * @param bytes_read : How many bytes were read.
 * @return 0 in case of success, error code otherwise
 */
// MARK:  SPI Mutex Required
int8_t usb_read(uint8_t address, uint8_t endpoint, uint16_t length,
		uint8_t* buf, uint8_t* bytes_read, uint16_t nak_limit)
{
	uint8_t rcode, j;
	uint8_t pktsize;
	uint8_t xfrlen = 0;

	set_max3421_address(address);	// 56us was 109us

	//set toggle value (use endpoint - 1 because array starts at 0)
//	max3421e_write(rHCTL, (usb_device[address].ep[endpoint-1].rcvToggle) ? bmRCVTOG1 : bmRCVTOG0);
//	max3421e_write(rHCTL, (pep->bmRcvToggle) ? bmRCVTOG1 : bmRCVTOG0);

	while (1) /* use a 'return' to exit this loop.*/
	{
		/* Start IN transfer*/
		rcode = dispatch_packet(tokIN, endpoint, nak_limit);

//		if (rcode == hrTOGERR)
//		{
			/* Yes, we flip the receive toggle wrong here so that next time it is actually correct!*/
//			usb_device[address].ep[endpoint-1].rcvToggle =
//					(max3421e_read(rHRSL) & bmRCVTOGRD) ? bmRCVTOG0 : bmRCVTOG1;
//			max3421e_write(rHCTL, usb_device[address].ep[endpoint-1].rcvToggle); /* set toggle value*/
//			continue;
//		}

		if (rcode != hrSUCCESS)
		{
			if (rcode != hrNAK)
			{
				return -1;
			}

			return rcode;
		}

		pktsize = max3421e_read(rRCVBC); /* get the number of received bytes available*/

		/* grab the data*/
		for (j = 0; j < pktsize; j++)
			buf[j + xfrlen] = max3421e_read(rRCVFIFO);

		max3421e_write(rHIRQ, bmRCVDAVIRQ); /* Clear the IRQ*/

		xfrlen += pktsize; /* add this packet's byte count to total transfer length */

		/*
		 * Check if we're done reading. Either we've received a 'short' packet (<maxPacketSize),
		 * or the desired number of bytes has been transferred.
		 */
		if ((pktsize == 0) || (xfrlen >= length))
		{
			// Remember the toggle value for the next transfer.
//			usb_device[address].ep[endpoint-1].rcvToggle =
//					(max3421e_read(rHRSL) & bmRCVTOGRD) ? bmRCVTOG1 : bmRCVTOG0;
			*bytes_read = xfrlen;
			// Break out of the loop.
			break;
		}
	}
	return (rcode);
}

// MARK:  SPI Mutex Required
int8_t usb_write(uint8_t address, uint8_t endpoint, uint16_t length, uint8_t* data,
		uint16_t nak_limit)
{
	uint8_t rcode = hrSUCCESS, retry_count;
	uint8_t *data_p = data; /* local copy of the data pointer*/
	uint16_t bytes_to_send, nak_count;
	uint16_t bytes_left = length;
	TickType_t timeout = xTaskGetTickCount() + USB_XFER_TIMEOUT /1;// ((float) 1000/configTICK_RATE_HZ);

	set_max3421_address(address);	// 56us was 109us

	while ((bytes_left) && (timeout > xTaskGetTickCount()))
	{
		retry_count = 0;
		nak_count = 0;
		bytes_to_send =
				(bytes_left >= usb_device[address].ep_descriptor[endpoint - 1].wMaxPacketSize) ?
						usb_device[address].ep_descriptor[endpoint - 1].wMaxPacketSize : bytes_left;


		/*Setup the data toggles*/
		if(usb_device[address].ep_info[endpoint - 1].sndToggle == bmSNDTOG0)
		{
			max3421e_write(rHCTL,bmSNDTOG0);
			usb_device[address].ep_info[endpoint - 1].sndToggle = bmSNDTOG1;
		}
		else
		{
			max3421e_write(rHCTL,bmSNDTOG1);
			usb_device[address].ep_info[endpoint - 1].sndToggle = bmSNDTOG0;
		}

		max3421e_write_bytes(rSNDFIFO, bytes_to_send, data_p); /* filling output FIFO*/
		max3421e_write(rSNDBC, bytes_to_send); /* set number of bytes*/
		max3421e_write(rHXFR, (tokOUT | endpoint)); /* dispatch packet*/

		/* Wait for interrupt*/
		while (timeout > xTaskGetTickCount())
		{
			if (max3421e_read(rHIRQ) & bmHXFRDNIRQ)
			{
				/* Clear the interrupt.*/
				max3421e_write(rHIRQ, bmHXFRDNIRQ);
				break;
			}
		}

#if 1
		/* Wait for HRSL*/
		while (timeout > xTaskGetTickCount())
		{
			rcode = (max3421e_read(rHRSL) & 0x0f);
			if (rcode != hrBUSY)
				break;
//			else
//				usb_host_print("USB dispatch busy!\r\n");
		}
#else
		rcode = (max3421e_read(rHRSL) & 0x0f);
#endif
		while (rcode && (timeout > xTaskGetTickCount()))
		{
			switch (rcode)
			{
				case hrSUCCESS:
					return (rcode);
					break;
				case hrNAK:
					nak_count++;
					if (nak_count > nak_limit)
						goto breakout;
					break;
				case hrTIMEOUT:
					retry_count++;
					if (retry_count == USB_RETRY_LIMIT)
						goto breakout;
					break;
				case hrTOGERR:
					// yes, we flip it wrong here so that next time it is actually correct!
#if 0
					usb_device[address].ep[endpoint-1]->sndToggle =
							(max3421e_read(rHRSL) & bmSNDTOGRD) ? 0 : 1;
					max3421e_write(rHCTL,
							(usb_devices[address].ep[endpoint-1]->sndToggle) ? bmSNDTOG1 : bmSNDTOG0); //set toggle value
#endif
					break;
				default:
					goto breakout;
			}

			/* process NAK according to Host out NAK bug */
			max3421e_write(rSNDBC, 0);
			max3421e_write(rSNDFIFO, *data_p);
			max3421e_write(rSNDBC, bytes_to_send);
			max3421e_write(rHXFR, (tokOUT | endpoint)); //dispatch packet

			/* Wait for interrupt*/
			while (timeout > xTaskGetTickCount())
			{
				if (max3421e_read(rHIRQ) & bmHXFRDNIRQ)
				{
					/* Clear the interrupt.*/
					max3421e_write(rHIRQ, bmHXFRDNIRQ);
					break;
				}
			}
			rcode = (max3421e_read(rHRSL) & 0x0f);
		}
		bytes_left -= bytes_to_send;
		data_p += bytes_to_send;
	}
	breakout: return (rcode); //should be 0 in all cases
}

/**
 * Read data from the control endpoint of a device.
 *
 * @param address USB device address.
 * @param pSUD pointer to 8-byte SETUP data.
 * @param buf pointer to data buffer.
 * @return 0 in case of success, error code otherwise
 */
// MARK:  SPI Mutex Required
uint8_t control_read(uint8_t address, uint8_t *pSUD, uint8_t* buf)
{
	uint8_t rcode;
	uint16_t bytes_to_read;
	bytes_to_read = pSUD[6] + 256 * pSUD[7];

	uint8_t bytes_read = 0;
	uint8_t * ptr = &bytes_read;
	uint8_t spi_tx_data[9];

	/* Put data in fifo*/
	spi_tx_data[0] = rSUDFIFO + 2;
	memcpy(spi_tx_data + 1, pSUD, 8);
	spi_transfer(SPI_MAX3421_NO_READ,
			spi_tx_data, 9);

	rcode = dispatch_packet(tokSETUP, 0, USB_NAK_LIMIT); /* SETUP packet to EP0 */
	if (rcode)
		return (rcode); /* should be 0, indicating ACK. Else return error code.*/

	/* One or more IN packets (may be a multi-packet transfer)*/
	max3421e_write(rHCTL, bmRCVTOG1); /* FIRST Data packet in a CTL transfer uses DATA1 toggle.*/
//	usb_device[address].ep[0].rcvToggle = bmRCVTOG1;

	rcode = usb_read(address, 0, bytes_to_read, buf, ptr, USB_NAK_LIMIT);
	if (rcode)
	{
//		if (rcode == hrTOGERR)
//		{
//			// yes, we flip it wrong here so that next time it is actually correct!
//			usb_device[address].ep[0].rcvToggle = ((max3421e_read(rHRSL) & bmRCVTOGRD)) ? 1 : 0;
//		}
		return (rcode);
	}
	rcode = dispatch_packet(tokOUTHS, 0, USB_NAK_LIMIT);
	if (rcode)
		return (rcode); /* should be 0, indicating ACK. Else return error code.*/
	return (0); /* success!*/
}

// MARK:  SPI Mutex Required
uint8_t control_write(uint8_t *pSUD)
{
	uint8_t rcode;
	uint8_t spi_tx_data[9];

	spi_tx_data[0] = rSUDFIFO + 2;
	memcpy(spi_tx_data + 1, pSUD, 8);
	spi_transfer(SPI_MAX3421_NO_READ, spi_tx_data, 9);

	/* 1. Send the SETUP token and 8 setup bytes. Device should immediately ACK.*/
	rcode = dispatch_packet(tokSETUP, 0, USB_NAK_LIMIT);  /* SETUP packet to EP0*/
	if (rcode)
		return (rcode);   /* should be 0, indicating ACK.*/

	/* 2. No data stage, so the last operation is to send an IN token to the peripheral
	 * 		as the STATUS (handshake) stage of this control transfer. We should get NAK or the
	 * 		DATA1 PID. When we get the DATA1 PID the 3421 automatically sends the closing ACK.
	 * */

	rcode = dispatch_packet(tokINHS, 0, USB_NAK_LIMIT); /* This function takes care of NAK retries.*/
	if (rcode)
		return (rcode);  /* should be 0, indicating ACK.*/
	else
		return (0);
}

/**
 * @brief Figure out the correct setting for the LOWSPEED and HUBPRE mode bits.
 * The HUBPRE bit needs to be set when MAX3421E operates at full speed, but it's
 * talking to a low-speed device (i.e., through a hub).  Setting that bit ensures
 * that every low-speed packet is preceded by a full-speed PRE PID.  Possible
 * configurations:
 *
 *  Hub speed:   Device speed:   =>      LOWSPEED bit:   HUBPRE bit:
 * 		FULL        FULL         =>      	0               0
 * 		FULL        LOW          =>      	1               1
 * 		LOW        	LOW          =>      	1               0
 * 		LOW        	FULL         =>      	1               0
 *
 * @param address : the address we want to set
 */
// MARK:  SPI Mutex Required
void set_max3421_address(uint8_t address)
{
	static uint8_t last_address = 0;

	if (last_address != address)
	{

		uint8_t bmHubPre = 0;
		max3421e_write(rPERADDR, address);
		uint8_t mode = max3421e_read(rMODE);
		if (usb_device[HUB_ADDRESS].is_hub)
		{
			bmHubPre = bmHUBPRE;
		}
		// Set bmLOWSPEED and bmHUBPRE in case of low-speed device, reset them otherwise
		max3421e_write(
		rMODE,
				(usb_device[address].lowspeed) ?
						mode | bmLOWSPEED | bmHubPre : mode & ~(bmHUBPRE | bmLOWSPEED));

	}
	last_address = address;
}

/**
 * Gets the device descriptor of a USB device.
 * @param device USB device
 * @param descriptor pointer to a usb_deviceDescriptor record that will be filled with the requested data.
 * @return 0 in case of success, error code otherwise
 */
// MARK:  SPI Mutex Required
uint8_t get_device_descriptors(uint8_t address)
{
	uint8_t rcode = 0;

	uint8_t Get_Descriptor_Device[8] =
	{ 0x80, 0x06, 0x00, USB_DESCRIPTOR_DEVICE, 0x00, 0x00, 18, 0x00 };
	rcode = control_read(address, Get_Descriptor_Device,
			(uint8_t*) &usb_device[address].dev_descriptor);
	return rcode;
}

/**
 * @brief Set the address of the device connected.
 *
 * @param address : The address we want to set for the device.
 * @return 0 in case of success, error code otherwise
 */
// MARK:  SPI Mutex Required
uint8_t set_device_address(uint8_t address)
{
	uint8_t rcode = 0;

	uint8_t Set_Address[8] =
	{ 0x00, 0x05, address, 0x00, 0x00, 0x00, 0x00, 0x00 };

	rcode = control_write(Set_Address);   // CTL-Write, no data stage

	return rcode;
}

/**
 * @brief After we have received the device descriptor, we are ready to get the rest
 * of the descriptors to fill the structures that have all the details on the device.
 * @param address : The address we want to set for the device.
 * @return 0 in case of success, error code otherwise
 */
// MARK:  SPI Mutex Required
uint8_t get_rest_of_discriptors(uint8_t address)
{
	uint8_t rcode = 0;
	uint8_t length = 9;

	/* When the configuration descriptor is read, it returns the entire configuration
	 * hierarchy which includes all related interface and endpoint descriptors. The wTotalLength
	 * field reflects the number of bytes in the hierarchy
	 */

	/* First get 9 bytes to get the total length
	 * 0x80 0x06 0x00 0x02 0x00 0x00 0x09 0x00
	 */
	uint8_t Get_Descriptor_Config[8] =
	{ USB_SETUP_DEVICE_TO_HOST, USB_REQUEST_GET_DESCRIPTOR, 0x00,
	USB_DESCRIPTOR_CONFIGURATION, 0x00, 0x00, length, 0x00 };

	rcode = control_read(address, Get_Descriptor_Config,
			(uint8_t*) &usb_device[address].config_descriptor);

	if (rcode)
	{
		error_print(
				"ERROR: Can't get USB host configuration descriptor address %d\n",
				address);
		return rcode;
	}

	length = usb_device[address].config_descriptor.wTotalLength;
	Get_Descriptor_Config[6] = length;

	uint8_t buf2[length]; /*Temp buffer to hold all the data we read*/

	/* now that we know the total length, lets get all of it*/
	rcode = control_read(address, Get_Descriptor_Config, (uint8_t*) &buf2);
	if (rcode)
	{
		error_print(
				"ERROR: Can't get USB host configuration descriptor address %d\n",
				address);
		return rcode;
	}
	else
	{
		/* USB_CONFIGURATION_DESCRIPTOR*/
		uint8_t ptr = 0;
		uint8_t size = buf2[0]; // size of USB_CONFIGURATION_DESCRIPTOR;
		memcpy((uint8_t*) &usb_device[address].config_descriptor, &buf2[ptr],
				size);

		/* USB_INTERFACE_DESCRIPTOR*/
		ptr += size;
		size = buf2[ptr]; // size of USB_INTERFACE_DESCRIPTOR;
		memcpy((uint8_t*) &usb_device[address].interface_descriptor, &buf2[ptr],
				size);

		switch (usb_device[address].interface_descriptor.bInterfaceClass)
		{
		case USB_CLASS_USE_CLASS_INFO:
			break;

		case USB_CLASS_HID:
			// USB_HID_DESCRIPTOR
			ptr += size;
			size = buf2[ptr]; // size of USB_HID_DESCRIPTOR;
			memcpy(&usb_device[address].hid_descriptor, &buf2[ptr], size);

            usb_device[address].hid_data.hid_in_ep = -1;
            usb_device[address].hid_data.hid_out_ep = -1;

			// ENDPOINTS
			for (int i = 0;
					i < usb_device[address].interface_descriptor.bNumEndpoints;
					++i)
			{
				if (i >= MAX_NUM_EP)
				{
					error_print("ERROR: Too many endpoints\r\n");
					return 1;
				}

				// USB_ENDPOINT_DESCRIPTOR
				ptr += size;
				size = buf2[ptr]; // size of USB_ENDPOINT_DESCRIPTOR;
				memcpy(&usb_device[address].ep_descriptor[i], &buf2[ptr], size);

                const uint8_t EP_NUMBER = usb_device[address].ep_descriptor[i].bEndpointAddress & 0x0F;

                // Locating IN and OUT endpoints for non-standard joysticks.
                // For interrupt endpoints...
                if (usb_device[address].ep_descriptor[i].bmAttributes == 0b000000011) {
                    if (usb_device[address].ep_descriptor[i].bEndpointAddress & 0x80) {
                        // IN direction
                        if (usb_device[address].hid_data.hid_in_ep == -1) {
                            usb_device[address].hid_data.hid_in_ep = EP_NUMBER;
                        }
                    } else {
                        // OUT direction
                        if (usb_device[address].hid_data.hid_out_ep == -1) {
                            usb_device[address].hid_data.hid_out_ep = EP_NUMBER;
                        }
                    }
                }
			}

			// data toggle is reset to '0' at Set_Configuration request on all
			// interrupt and bulk endpoints in enumeration
			usb_host_print("\r\n Setting Configuration\r\n");
			rcode = set_configuration(
					usb_device[address].config_descriptor.bConfigurationValue);

			uint8_t protocol = 0;
			uint8_t interface = 0;
			rcode = set_protocol(protocol, interface);

			if (rcode)
			{
				error_print("ERROR: Can't set USB host configuration\r\n");
				// TODO report error
				return rcode;
			}

			if(usb_device[HUB_ADDRESS].is_hub)
				debug_print("HID device on hub port %d enumerated correctly.\r\n", address-1);
			else
				debug_print("HID device %d enumerated correctly.\r\n", address);
			break;

		case USB_CLASS_HUB:
			/* only one HUB supported in this release*/
			if (!usb_device[HUB_ADDRESS].is_hub)
			{

				usb_host_print("\n Setting Configuration\n");
				rcode =
						set_configuration(
								usb_device[HUB_ADDRESS].config_descriptor.bConfigurationValue);

				/* address should always be 1 for hub*/
				debug_print("\n** HUB Device Detected ** \n");
				rcode = GetHubDescriptor(address);
				if (rcode)
				{
					error_print("ERROR: Can't get GET_HUB_DESCRIPTOR\r\n");
					return rcode;
				}

				/* power all the hub ports*/
				rcode = Hub_ports_pwr();

				if (rcode)
				{
					error_print("ERROR: Can't set HUB power\r\n");
					return rcode;
				}
				else
				{
					usb_device[address].hub.PollEnable = true;
					usb_device[HUB_ADDRESS].is_hub = true;
				}
			}
			else
			{
				error_print("ERROR: Only 1 HUB supported\n");
			}
			break;

		case USB_CLASS_VENDOR_SPECIFIC:
			debug_print("** Vendor Specific Device Detected ** \r\n");
			break;

		default:
			break;
		}
	}
	return rcode;
}

