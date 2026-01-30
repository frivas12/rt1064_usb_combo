/**
 * @file usb_device.c
 *
 * @brief Functions for USB devices such as creating the task for the device,
 * try to enumerate, and poll the device.
 *
 *
 */

#include "usb_device.hh"

#include <asf.h>

#include "Debugging.h"
#include "UsbCore.h"
#include "hid-mapping.hh"
#include "hid.h"
#include "hid_out.h"
#include "hid_pid_fixes.h"
#include "itc-service.hh"
#include "lock_guard.hh"
#include "max3421e.h"
#include "max3421e_usb.h"
#include "spi-transfer-handle.hh"
#include "string.h"
#include "sys_task.h"
#include "usb_host.h"
#include "usb_hub.h"

extern SemaphoreHandle_t xSPI_Semaphore;

/****************************************************************************
 * Private Data
 ****************************************************************************/
UsbDevice usb_device[USB_NUMDEVICES];
UsbNewDevice* pDev[USB_NUMDEVICES];

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static void print_device_info(uint8_t address);
static void suspend_devices(uint8_t except);
static void resume_devices(uint8_t except);
static int8_t enumerate(UsbNewDevice* pNew_device);
static void task_usb_device(UsbNewDevice* pNew_device);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/**
 * @brief Prints the device information to the debug terminal in SystemView
 *
 * @param address : The port address for the device.
 */
static void print_device_info(uint8_t address) {
    usb_host_print("\r\nGetting device descriptors for %d.\r\n", address);
    usb_host_print("bDeviceClass:	0x%x\r\n",
                   usb_device[address].dev_descriptor.bDeviceClass);
    usb_host_print("bDeviceSubClass:	0x%x\r\n",
                   usb_device[address].dev_descriptor.bDeviceSubClass);
    usb_host_print("idVendor:		0x%x\r\n",
                   usb_device[address].dev_descriptor.idVendor);
    usb_host_print("idProduct:		0x%x\r\n",
                   usb_device[address].dev_descriptor.idProduct);
    usb_host_print("bNumConfigurations:	0x%x\r\n",
                   usb_device[address].dev_descriptor.bNumConfigurations);

    /* USB_CONFIGURATION_DESCRIPTOR*/
    usb_host_print("\r\nCONFIGURATION Descriptor:\r\n");
    usb_host_print("wTotalLength:	0x%x\r\n",
                   usb_device[address].config_descriptor.wTotalLength);
    usb_host_print("bNumInterfaces:	0x%x\r\n",
                   usb_device[address].config_descriptor.bNumInterfaces);
    usb_host_print("bConfigValue:	0x%x\r\n",
                   usb_device[address].config_descriptor.bConfigurationValue);
    usb_host_print("iConfiguration:	0x%x\r\n",
                   usb_device[address].config_descriptor.iConfiguration);
    usb_host_print("bmAttributes:	0x%x",
                   usb_device[address].config_descriptor.bmAttributes);
    if (usb_device[address].config_descriptor.bmAttributes & 0x40) {
        usb_host_print(" (self-powered)\r\n");
    } else {
        usb_host_print(" (bus powered)\r\n",
                       usb_device[address].config_descriptor.bMaxPower * 2);
    }
    usb_host_print("bMaxPower:		0x%x",
                   usb_device[address].config_descriptor.bMaxPower);
    usb_host_print(" (%d Ma)\r\n",
                   usb_device[address].config_descriptor.bMaxPower * 2);

    /* USB_INTERFACE_DESCRIPTOR*/
    usb_host_print("\r\nINTERFACE Descriptor:\r\n");
    usb_host_print("bInterfaceNumber:	0x%x\r\n",
                   usb_device[address].interface_descriptor.bInterfaceNumber);
    usb_host_print("bAlternateSetting:	0x%x\r\n",
                   usb_device[address].interface_descriptor.bAlternateSetting);
    usb_host_print("bNumEndpoints:	0x%x\r\n",
                   usb_device[address].interface_descriptor.bNumEndpoints);
    usb_host_print("bInterfaceClass:	0x%x",
                   usb_device[address].interface_descriptor.bInterfaceClass);
    if (usb_device[address].interface_descriptor.bInterfaceClass ==
        USB_CLASS_HID) {
        usb_host_print(" (HID)\r\n");
    } else if (usb_device[address].interface_descriptor.bInterfaceClass ==
               USB_CLASS_VENDOR_SPECIFIC) {
        usb_host_print("  (Vendor Specific)\r\n");
    } else if (usb_device[address].interface_descriptor.bInterfaceClass ==
               USB_CLASS_HUB) {
        usb_host_print("  (HUB)\r\n");
    } else {
        usb_host_print(
            "bInterfaceClass:		0x%x \r\n",
            usb_device[address].interface_descriptor.bInterfaceClass);
    }
    usb_host_print("bInterfaceSubClass:	0x%x\r\n",
                   usb_device[address].interface_descriptor.bInterfaceSubClass);
    usb_host_print("bInterfaceProtocol:	0x%x\r\n",
                   usb_device[address].interface_descriptor.bInterfaceProtocol);
    usb_host_print("iInterface:		0x%x\r\n",
                   usb_device[address].interface_descriptor.iInterface);

    switch (usb_device[address].interface_descriptor.bInterfaceClass) {
    case USB_CLASS_HID:
        usb_host_print("\r\n ** HID Device Detected ** \r\n");
        usb_host_print("\r\nHID Descriptor:\r\n");
        usb_host_print("bcdHID:		0x%x\r\n",
                       usb_device[address].hid_descriptor.bcdHID);
        usb_host_print("bCountryCode:	0x%x\r\n",
                       usb_device[address].hid_descriptor.bCountryCode);
        usb_host_print("bNumDescriptors:	0x%x\r\n",
                       usb_device[address].hid_descriptor.bNumDescriptors);
        usb_host_print("bDescrType:		0x%x\r\n",
                       usb_device[address].hid_descriptor.bDescrType);
        ;
        usb_host_print(
            "wDescriptorLength:	0x%x\r\n",
            usb_device[address].hid_descriptor.wDescriptorLength +
                (usb_device[address].hid_descriptor.wDescriptorLengthh << 8));

        /* ENDPOINTS*/
        for (int i = 0;
             i < usb_device[address].interface_descriptor.bNumEndpoints; ++i) {
            usb_host_print("\r\nEndpoint Descriptor:\r\n");
            usb_host_print(
                "Endpoint Address:	%d",
                (usb_device[address].ep_descriptor[i].bEndpointAddress & 0x0F));
            if (usb_device[address].ep_descriptor[i].bEndpointAddress & 0x80) {
                usb_host_print("   IN\r\n");
            } else
                usb_host_print("   OUT\r\n");

            usb_host_print("Transfer Type:");
            switch (usb_device[address].ep_descriptor[i].bmAttributes & 0x03) {
            case USB_TRANSFER_TYPE_CONTROL:
                usb_host_print("		CONTROL\r\n");
                break;
            case USB_TRANSFER_TYPE_ISOCHRONOUS:
                usb_host_print("		ISOCHRONOUS\r\n");
                break;
            case USB_TRANSFER_TYPE_BULK:
                usb_host_print("		BULK\r\n");
                break;
            case USB_TRANSFER_TYPE_INTERRUPT:
                usb_host_print("		INTERRUPT\r\n");
                break;
            }
            usb_host_print("bInterval:		0x%x",
                           usb_device[address].ep_descriptor[i].bInterval);
            usb_host_print(" (%d msec.)\r\n",
                           usb_device[address].ep_descriptor[i].bInterval);
            usb_host_print("wMaxPacketSize:	0x%x\r\n",
                           usb_device[address].ep_descriptor[i].wMaxPacketSize);

            break;
        }
    }
}

/**
 * @brief Suspend all USB host devices other than the port pass in function
 * 			including the host task.  The host task is suspended last so or it
 * 			would not be running to suspend the other task.
 * @param except : The port of the device we don't want to suspend.
 */
static void suspend_devices(uint8_t except) {
    /*Suspend all the devices except the one pass in the function*/
    for (uint8_t i = 1; i < USB_NUMDEVICES; ++i) {
        if (xDeviceHandle[i] != NULL) {
            if (i != except) vTaskSuspend(xDeviceHandle[i]);
        }
    }

    /* Suspend the Host task */
    if (xDeviceHandle[0] != NULL) {
        vTaskSuspend(xDeviceHandle[0]);
    }
}

/**
 * @brief Resume all USB host devices other than the port pass in function
 * 			including the host task.
 * @param except : The port of the device we don't want to suspend.
 */
static void resume_devices(uint8_t except) {
    /*Resume the host task*/
    if (xDeviceHandle[0] != NULL) {
        vTaskResume(xDeviceHandle[0]);
    }

    /*Resume all the devices that were suspended*/
    for (uint8_t i = 1; i < USB_NUMDEVICES; ++i) {
        if (xDeviceHandle[i] != NULL) {
            if (i != except) vTaskResume(xDeviceHandle[i]);
        }
    }
}

/**
 * @brief Try to enumerate the newly connected device.
 *
 * @param pDev : The device structure passed from when the connection was made.
 *
 * @return : 0 if everything went well, else error
 */
// MARK:  SPI Mutex Required
int8_t enumerate(UsbNewDevice* pDev) {
    int8_t rcode;
    int8_t address         = pDev->address;
    usb_device[0].lowspeed = pDev->lowspeed;

    /* First request goes to address 0*/
    set_max3421_address(0);

    /* Get device descriptor to see what type of device is connected.  HID & CDC
     * have same class so we will have to look at the interface descriptor to
     * get the type.
     * */
    rcode = get_device_descriptors(0);
    if (rcode) {
        error_print("ERROR: Getting USB host device descriptor\n");
        return rcode;
    }

    rcode = set_device_address(address); /* root port always address = 1*/
    if (rcode) {
        error_print("ERROR: Could not set device %d address.\r\n", address);
        return rcode;
    }

    usb_device[address].lowspeed = usb_device[0].lowspeed;
    set_max3421_address(address);

    xSemaphoreGive(xSPI_Semaphore);
    vTaskDelay(
        pdMS_TO_TICKS(200)); /* Older spec says you should wait at least 200ms*/
    xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);

    rcode = get_device_descriptors(address);
    if (rcode) {
        error_print("ERROR: Getting USB host device descriptor\n");
        return rcode;
    }

    rcode = get_rest_of_discriptors(address);
    if (rcode) {
        error_print("ERROR: USB Host root port device enumeration failed.\r\n");
        usb_device[address].enumerated = 0;
        return rcode;
    } else {
        usb_device[address].enumerated = 1;
    }
    usb_device[address].initialized = 0;

    return (0); /* if we get here we had success!*/
}

/****************************************************************************
 * Task
 ****************************************************************************/
/**
 * @brief This is the task for devices connected to the root USB port or hub
 * ports. It will try to enumerate the device and then poll the device at its
 * given interval.
 *
 * @param pNew_device : Structure passed when the task was created.  Hold the
 * addresses and speed along with other details needed to enumerate the device.
 */
static void task_usb_device(UsbNewDevice* pNew_device) {
    uint8_t hid_report_length;

    int8_t rcode;
    int8_t address = pNew_device->address;

    /* Reset the structure*/
    memset(&usb_device[address], 0, sizeof(UsbDevice));

    // Only for the shuttlExpress joystick
    // Set the slot_select to slot none
    usb_device[address].hid_data.slot_select = 0;

    //	em_stop(EM_STOP_ALL);	// send stop signal to all slots

    xSemaphoreTake(xSPI_Semaphore, portMAX_DELAY);
    rcode = enumerate(pNew_device); /*Try to enumerate the device*/
    xSemaphoreGive(xSPI_Semaphore);
    if (rcode != hrSUCCESS) {
        resume_devices(
            address); /*Resume all the other device task that were suspended*/
        remove_usb_device(address); /*Remove this task and is memory because it
                                       failed to enumerate*/
    }

    /*Setup the task delay from the interval at which we need to poll the
     * device*/
    TickType_t xLastWakeTime;
    // Initialize the xLastWakeTime variable with the current time.
    xLastWakeTime = xTaskGetTickCount();
    const TickType_t xTicksToDelay =
        usb_device[address].ep_descriptor[0].bInterval /
        ((float)1000 / configTICK_RATE_HZ);

    print_device_info(address);

    /* If it is a hub, delete the task because we don't need it.  The host task
     * will poll the port on the hub*/
    if ((usb_device[HUB_ADDRESS].is_hub) && (address == HUB_ADDRESS)) {
        resume_devices(
            address); /*Resume all the other device task that were suspended*/

        // Use the handle to delete the task.
        if (xDeviceHandle[address] != NULL) {
            xDeviceHandle[address] = NULL;
            vTaskDelete(xDeviceHandle[address]);
        }
    }

    /* As of now, only HID devices are implemented so any other type just delete
     * the task. This could also be a second hub which we will delete because we
     * currently only support one hub.*/
    if (usb_device[address].interface_descriptor.bInterfaceClass !=
        USB_CLASS_HID) {
        resume_devices(
            address); /*Resume all the other device task that were suspended*/
        remove_usb_device(address);
    }

    /*If the device is an HID, get the report*/
    hid_report_length =
        usb_device[address].hid_descriptor.wDescriptorLength +
        (usb_device[address].hid_descriptor.wDescriptorLengthh << 8);

    rcode = HID_get_report(address, hid_report_length);
    fix_hid_report_parsing_errors(address);

    /*Resume all the other device task that were suspended*/
    resume_devices(address);
    if (rcode != hrSUCCESS) {
        remove_usb_device(address);
    }

    // Create the OUT queue iff the hid_out_ep is defined.
    std::optional<
        service::itc::queue_handle<service::itc::message::hid_out_message>>
        maybe_out_queue;
    if (usb_device[address].hid_data.hid_out_ep != -1) {
        maybe_out_queue =
            service::itc::pipeline_hid_out().create_queue(address);
    }
    auto out_state = drivers::usb_device::out_report_state(address);

    // Reloads from EEPROM on connect.
    {
        auto handle = service::hid_mapping::address_handle::create(address);
        drivers::spi::handle_factory spi;
        handle.load_in_from_eeprom(spi);
        handle.load_out_from_eeprom(spi);
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    for (;;) {
        // Service the in and out data.
        {
            auto mappings =
                service::hid_mapping::address_handle::create(address);
            drivers::usb_device::service_in_report(usb_device[address],
                                                   mappings);
            if (maybe_out_queue) {
                drivers::usb_device::service_out_report(
                    usb_device[address], *maybe_out_queue, mappings, out_state);
            }
        }

        /* Wait for the next cycle base on bInterval.*/
        vTaskDelayUntil(&xLastWakeTime, xTicksToDelay);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/**
 * @brief 	A new device was detected. Now we need to try to enumerate it and
 * 			create a new task for this USB device.  We need to suspend the host
 * 			task that call this function and all the other devices enumerated
 * 			so they don't impede trying to enumerate this device.  Once this
 * 			new device has attempted to enumerate in the new device task, that
 * task will resume the host and othe rdevices.
 *
 * @param 	pNew_device : Handle for the new device.
 */
void usb_device_add_device_task(UsbNewDevice* pNew_device) {
    // uint8_t rcode;

    uint8_t address = pNew_device->address;

    pDev[address] =
        static_cast<UsbNewDevice*>(pvPortMalloc(sizeof(UsbNewDevice)));
    pDev[address]->address  = pNew_device->address;
    pDev[address]->lowspeed = pNew_device->lowspeed;

    char pcName[8]          = {'D', 'e', 'v', 'i', 'c', 'e', 'X'};
    pcName[6]               = pNew_device->address + 0x30;
    pNew_device->task_state = POLL;

    if (xDeviceHandle[pDev[pNew_device->address]->address] == NULL) {
        /* Create a new task to for the USB device connected */
        if (xTaskCreate((TaskFunction_t)task_usb_device, pcName,
                        TASK_USB_HOST_DEVICE_STACK_SIZE, pDev[address],
                        TASK_USB_HOST_DEVICE_STACK_PRIORITY,
                        &xDeviceHandle[pNew_device->address]) != pdPASS) {
            error_print("ERROR: Failed to create USB Host task\r\n");
        }
        /*Suspend the host so that other devices on hub so that other device
         * don't try to enumerate until this device has completed enumeration.
         * Once it has enumerated, this device will resume the host task*/
        suspend_devices(address);
    }
}
