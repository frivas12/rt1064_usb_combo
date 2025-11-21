/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016 - 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "usb_host_config.h"
#include "usb_host.h"
#include "usb_host_hid.h"
#include "host_hid_generic.h"
#include "app.h"
#include "spi_bridge.h"

#include <string.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief process hid data and print generic data.
 *
 * @param genericInstance   hid generic instance pointer.
 */
static void USB_HostHidGenericProcessBuffer(usb_host_hid_generic_instance_t *genericInstance);
static void USB_HostHidGenericPrintHex(usb_host_hid_generic_instance_t *genericInstance,
                                       const char *label,
                                       const uint8_t *data,
                                       uint32_t length);

/*!
 * @brief host hid generic control transfer callback.
 *
 * This function is used as callback function for control transfer .
 *
 * @param param      the host hid generic instance pointer.
 * @param data       data buffer pointer.
 * @param dataLength data length.
 * @status           transfer result status.
 */
static void USB_HostHidControlCallback(void *param, uint8_t *data, uint32_t dataLength, usb_status_t status);

/*!
 * @brief host hid generic interrupt in transfer callback.
 *
 * This function is used as callback function when call USB_HostHidRecv .
 *
 * @param param      the host hid generic instance pointer.
 * @param data       data buffer pointer.
 * @param dataLength data length.
 * @status           transfer result status.
 */
static void USB_HostHidInCallback(void *param, uint8_t *data, uint32_t dataLength, usb_status_t status);

/*!
 * @brief host hid generic interrupt out transfer callback.
 *
 * This function is used as callback function when call USB_HostHidSend .
 *
 * @param param    the host hid generic instance pointer.
 * @param data     data buffer pointer.
 * @param dataLength data length.
 * @status         transfer result status.
 */
static void USB_HostHidOutCallback(void *param, uint8_t *data, uint32_t dataLength, usb_status_t status);

/*!
 * @brief host hid generic prepare data for sending.
 *
 * @param genericInstance  the host hid generic instance pointer.
 *
 * @retval kStatus_USB_Sucess  there is data.
 * @retval kStatus_USB_Error   data is sent done.
 */
static usb_status_t USB_HostHidGenericPrepareOutData(usb_host_hid_generic_instance_t *genericInstance);

static usb_host_hid_generic_instance_t *USB_HostHidGenericAllocateInstance(void);
static usb_host_hid_generic_instance_t *USB_HostHidGenericFindInstanceByConfig(
    usb_host_configuration_handle configurationHandle);
static usb_host_hid_generic_instance_t *USB_HostHidGenericFindInstanceByDevice(usb_device_handle deviceHandle);
static void USB_HostHidGenericResetInstance(usb_host_hid_generic_instance_t *genericInstance);

/*******************************************************************************
 * Variables
 ******************************************************************************/

usb_host_hid_generic_instance_t g_HostHidGeneric[HID_GENERIC_MAX_DEVICES]; /* hid generic instances */
uint8_t testData[] =
    "Test string: This is usb host hid generic demo, it only support pid=0x00a2 and vid=0x1fc9 hid device. Host send "
    "this test string to device, device reply the data to host then host print the data\r\n";

USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t s_GenericInBuffer[HID_GENERIC_MAX_DEVICES][HID_GENERIC_IN_BUFFER_SIZE]; /*!< use to receive report descriptor and data */
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
uint8_t s_GenericOutBuffer[HID_GENERIC_MAX_DEVICES][HID_GENERIC_IN_BUFFER_SIZE]; /*!< use to send data */

#define SPI_DEVICE_ID_INVALID (0xFFU)

static void USB_HostHidGenericResetInstance(usb_host_hid_generic_instance_t *genericInstance)
{
    (void)memset(genericInstance, 0, sizeof(*genericInstance));
    genericInstance->deviceState = kStatus_DEV_Idle;
    genericInstance->prevState   = kStatus_DEV_Idle;
    genericInstance->runState    = kUSB_HostHidRunIdle;
    genericInstance->runWaitState = kUSB_HostHidRunIdle;
    genericInstance->deviceId     = SPI_DEVICE_ID_INVALID;
}

static usb_host_hid_generic_instance_t *USB_HostHidGenericAllocateInstance(void)
{
    for (uint8_t index = 0U; index < HID_GENERIC_MAX_DEVICES; ++index)
    {
        if (g_HostHidGeneric[index].deviceHandle == NULL)
        {
            USB_HostHidGenericResetInstance(&g_HostHidGeneric[index]);
            return &g_HostHidGeneric[index];
        }
    }
    return NULL;
}

static usb_host_hid_generic_instance_t *USB_HostHidGenericFindInstanceByConfig(
    usb_host_configuration_handle configurationHandle)
{
    for (uint8_t index = 0U; index < HID_GENERIC_MAX_DEVICES; ++index)
    {
        if (g_HostHidGeneric[index].configHandle == configurationHandle)
        {
            return &g_HostHidGeneric[index];
        }
    }
    return NULL;
}

static usb_host_hid_generic_instance_t *USB_HostHidGenericFindInstanceByDevice(usb_device_handle deviceHandle)
{
    for (uint8_t index = 0U; index < HID_GENERIC_MAX_DEVICES; ++index)
    {
        if (g_HostHidGeneric[index].deviceHandle == deviceHandle)
        {
            return &g_HostHidGeneric[index];
        }
    }
    return NULL;
}

/*******************************************************************************
 * Code
 ******************************************************************************/

static void USB_HostHidGenericProcessBuffer(usb_host_hid_generic_instance_t *genericInstance)
{
    genericInstance->genericInBuffer[genericInstance->inMaxPacketSize] = 0;

    USB_HostHidGenericPrintHex(genericInstance, "Input report", genericInstance->genericInBuffer,
                               genericInstance->lastInDataLength);

    if ((genericInstance->lastInDataLength > 0U) && genericInstance->deviceAnnounced)
    {
        (void)SPI_BridgeSendReport(genericInstance->deviceId, true, 0U, genericInstance->genericInBuffer,
                                   genericInstance->lastInDataLength);
    }
}

static void USB_HostHidGenericPrintHex(usb_host_hid_generic_instance_t *genericInstance,
                                       const char *label,
                                       const uint8_t *data,
                                       uint32_t length)
{
    if ((data == NULL) || (length == 0U))
    {
        return;
    }

    usb_echo("%s (vid=0x%x pid=0x%x hub=%u port=%u hs hub=%u hs port=%u level=%u, %u bytes):\r\n",
             label, genericInstance->vid, genericInstance->pid, genericInstance->hubNumber,
             genericInstance->portNumber, genericInstance->hsHubNumber, genericInstance->hsHubPort,
             genericInstance->level, (unsigned int)length);

    for (uint32_t offset = 0; offset < length; offset += 16U)
    {
        uint32_t remaining = length - offset;
        uint32_t lineSize  = (remaining > 16U) ? 16U : remaining;

        usb_echo("  %03u: ", (unsigned int)offset);
        for (uint32_t index = 0; index < lineSize; ++index)
        {
            usb_echo("%02x ", data[offset + index]);
        }
        usb_echo("\r\n");
    }
}

static void USB_HostHidControlCallback(void *param, uint8_t *data, uint32_t dataLength, usb_status_t status)
{
    usb_host_hid_generic_instance_t *genericInstance = (usb_host_hid_generic_instance_t *)param;

    if (kStatus_USB_TransferStall == status)
    {
        usb_echo("device don't support this ruquest \r\n");
    }
    else if (kStatus_USB_Success != status)
    {
        usb_echo("control transfer failed\r\n");
    }
    else
    {
    }

    if (genericInstance->runWaitState == kUSB_HostHidRunWaitSetInterface) /* set interface finish */
    {
        genericInstance->runState = kUSB_HostHidRunSetInterfaceDone;
    }
    else if (genericInstance->runWaitState == kUSB_HostHidRunWaitSetIdle) /* hid set idle finish */
    {
        genericInstance->runState = kUSB_HostHidRunSetIdleDone;
    }
    else if (genericInstance->runWaitState ==
             kUSB_HostHidRunWaitGetReportDescriptor) /* hid get report descriptor finish */
    {
        genericInstance->runState = kUSB_HostHidRunGetReportDescriptorDone;
    }
    else if (genericInstance->runWaitState == kUSB_HostHidRunWaitSetProtocol) /* hid set protocol finish */
    {
        genericInstance->runState = kUSB_HostHidRunSetProtocolDone;
    }
    else
    {
    }
}

static void USB_HostHidInCallback(void *param, uint8_t *data, uint32_t dataLength, usb_status_t status)
{
    usb_host_hid_generic_instance_t *genericInstance = (usb_host_hid_generic_instance_t *)param;

    if (genericInstance->runWaitState == kUSB_HostHidRunWaitDataReceived)
    {
        if (status == kStatus_USB_Success)
        {
            genericInstance->lastInDataLength = dataLength;
            genericInstance->runState = kUSB_HostHidRunDataReceived; /* go to process data */
        }
        else
        {
            genericInstance->lastInDataLength = 0U;
            if (genericInstance->deviceState == kStatus_DEV_Attached)
            {
                genericInstance->runState = kUSB_HostHidRunPrimeDataReceive; /* go to prime next receiving */
            }
        }
    }
}

static void USB_HostHidOutCallback(void *param, uint8_t *data, uint32_t dataLength, usb_status_t status)
{ /* NULL */
}

static usb_status_t USB_HostHidGenericPrepareOutData(usb_host_hid_generic_instance_t *genericInstance)
{
    uint16_t index = 0;

    if (genericInstance->sendIndex < (sizeof(testData) - 1)) /* sendIndex indicate the current position of testData */
    {
        /* get the max packet data, note: the data should be 0 when there is no actual data to send */
        for (index = 0; ((index + genericInstance->sendIndex) < (sizeof(testData) - 1)) &&
                        (index < genericInstance->outMaxPacketSize);
             ++index)
        {
            genericInstance->genericOutBuffer[index] = testData[index + genericInstance->sendIndex];
        }
        for (; index < genericInstance->outMaxPacketSize; ++index)
        {
            genericInstance->genericOutBuffer[index] = 0x00;
        }
        genericInstance->sendIndex += genericInstance->outMaxPacketSize;

        genericInstance->lastOutDataLength = index;
        USB_HostHidGenericPrintHex(genericInstance, "Output report", genericInstance->genericOutBuffer,
                                   genericInstance->lastOutDataLength);
        if ((genericInstance->lastOutDataLength > 0U) && genericInstance->deviceAnnounced)
        {
            (void)SPI_BridgeSendReport(genericInstance->deviceId, false, 0U, genericInstance->genericOutBuffer,
                                       genericInstance->lastOutDataLength);
        }

        return kStatus_USB_Success;
    }
    else
    {
        genericInstance->lastOutDataLength = 0U;
        return kStatus_USB_Error; /* there is no data to send */
    }
}

void USB_HostHidGenericTask(void *param)
{
    usb_status_t status;
    usb_host_hid_descriptor_t *hidDescriptor;
    uint32_t hidReportLength = 0;
    uint8_t *descriptor;
    uint32_t endPosition;
    usb_host_hid_generic_instance_t *genericInstance = (usb_host_hid_generic_instance_t *)param;

    if ((genericInstance == NULL) || (genericInstance->deviceHandle == NULL))
    {
        return;
    }

    /* device state changes, process once for each state */
    if (genericInstance->deviceState != genericInstance->prevState)
    {
        genericInstance->prevState = genericInstance->deviceState;
        switch (genericInstance->deviceState)
        {
            case kStatus_DEV_Idle:
                break;

            case kStatus_DEV_Attached: /* deivce is attached and numeration is done */
                genericInstance->runState = kUSB_HostHidRunSetInterface;
                if (USB_HostHidInit(genericInstance->deviceHandle, &genericInstance->classHandle) !=
                    kStatus_USB_Success)
                {
                    usb_echo("host hid class initialize fail\r\n");
                }
                else
                {
                    usb_echo("hid generic attached\r\n");
                }
                genericInstance->sendIndex = 0;
                genericInstance->lastInDataLength = 0U;
                genericInstance->lastOutDataLength = 0U;
                genericInstance->reportDescriptorLength = 0U;
                break;

            case kStatus_DEV_Detached: /* device is detached */
                genericInstance->deviceState = kStatus_DEV_Idle;
                genericInstance->runState    = kUSB_HostHidRunIdle;
                USB_HostHidDeinit(genericInstance->deviceHandle, genericInstance->classHandle);
                genericInstance->classHandle = NULL;
                usb_echo("hid generic detached\r\n");
                USB_HostHidGenericResetInstance(genericInstance);
                break;

            default:
                break;
        }
    }

    /* run state */
    switch (genericInstance->runState)
    {
        case kUSB_HostHidRunIdle:
            break;

        case kUSB_HostHidRunSetInterface: /* 1. set hid interface */
            genericInstance->runWaitState = kUSB_HostHidRunWaitSetInterface;
            genericInstance->runState     = kUSB_HostHidRunIdle;
            if (USB_HostHidSetInterface(genericInstance->classHandle, genericInstance->interfaceHandle, 0,
                                        USB_HostHidControlCallback, genericInstance) != kStatus_USB_Success)
            {
                usb_echo("set interface error\r\n");
            }
            break;

        case kUSB_HostHidRunSetInterfaceDone: /* 2. hid set idle */
            genericInstance->inMaxPacketSize =
                USB_HostHidGetPacketsize(genericInstance->classHandle, USB_ENDPOINT_INTERRUPT, USB_IN);
            genericInstance->outMaxPacketSize =
                USB_HostHidGetPacketsize(genericInstance->classHandle, USB_ENDPOINT_INTERRUPT, USB_OUT);

            /* first: set idle */
            genericInstance->runWaitState = kUSB_HostHidRunWaitSetIdle;
            genericInstance->runState     = kUSB_HostHidRunIdle;
            if (USB_HostHidSetIdle(genericInstance->classHandle, 0, 0, USB_HostHidControlCallback, genericInstance) !=
                kStatus_USB_Success)
            {
                usb_echo("Error in USB_HostHidSetIdle\r\n");
            }
            break;

        case kUSB_HostHidRunSetIdleDone: /* 3. hid get report descriptor */
            /* get report descriptor's length */
            hidDescriptor = NULL;
            descriptor    = (uint8_t *)((usb_host_interface_t *)genericInstance->interfaceHandle)->interfaceExtension;
            endPosition   = (uint32_t)descriptor +
                          ((usb_host_interface_t *)genericInstance->interfaceHandle)->interfaceExtensionLength;

            while ((uint32_t)descriptor < endPosition)
            {
                if (*(descriptor + 1) == USB_DESCRIPTOR_TYPE_HID) /* descriptor type */
                {
                    hidDescriptor = (usb_host_hid_descriptor_t *)descriptor;
                    break;
                }
                else
                {
                    descriptor = (uint8_t *)((uint32_t)descriptor + (*descriptor)); /* next descriptor */
                }
            }

            if (hidDescriptor != NULL)
            {
                usb_host_hid_class_descriptor_t *hidClassDescriptor;
                hidClassDescriptor = (usb_host_hid_class_descriptor_t *)&(hidDescriptor->bHidDescriptorType);
                for (uint8_t index = 0; index < hidDescriptor->bNumDescriptors; ++index)
                {
                    hidClassDescriptor += index;
                    if (hidClassDescriptor->bHidDescriptorType == USB_DESCRIPTOR_TYPE_HID_REPORT)
                    {
                        hidReportLength =
                            (uint16_t)USB_SHORT_FROM_LITTLE_ENDIAN_ADDRESS(hidClassDescriptor->wDescriptorLength);
                        break;
                    }
                }
            }
            if (hidReportLength > HID_GENERIC_IN_BUFFER_SIZE)
            {
                usb_echo("hid buffer is too small\r\n");
                genericInstance->runState = kUSB_HostHidRunIdle;
                return;
            }

            genericInstance->reportDescriptorLength = hidReportLength;

            if (!genericInstance->deviceAnnounced)
            {
                uint8_t deviceId;
                usb_host_interface_t *iface = (usb_host_interface_t *)genericInstance->interfaceHandle;
                status_t bridgeStatus =
                    SPI_BridgeAllocDevice(kSpiBridgeDeviceGenericHid, genericInstance->vid, genericInstance->pid,
                                          iface->interfaceDesc->bInterfaceNumber,
                                          (genericInstance->outMaxPacketSize > 0U), hidReportLength,
                                          genericInstance->hubNumber, genericInstance->portNumber,
                                          genericInstance->hsHubNumber, genericInstance->hsHubPort,
                                          genericInstance->level, &deviceId);
                if (bridgeStatus == kStatus_Success)
                {
                    genericInstance->deviceId        = deviceId;
                    genericInstance->deviceAnnounced = true;
                }
            }

            genericInstance->runWaitState = kUSB_HostHidRunWaitGetReportDescriptor;
            genericInstance->runState     = kUSB_HostHidRunIdle;
            /* second: get report descriptor */
            USB_HostHidGetReportDescriptor(genericInstance->classHandle, genericInstance->genericInBuffer,
                                           hidReportLength, USB_HostHidControlCallback, genericInstance);
            break;

        case kUSB_HostHidRunGetReportDescriptorDone: /* 4. hid set protocol */
            if (genericInstance->reportDescriptorLength > 0U)
            {
                USB_HostHidGenericPrintHex(genericInstance, "Report descriptor",
                                           genericInstance->genericInBuffer,
                                           genericInstance->reportDescriptorLength);
                if (genericInstance->deviceAnnounced && (genericInstance->deviceId != SPI_DEVICE_ID_INVALID))
                {
                    (void)SPI_BridgeSendReportDescriptor(genericInstance->deviceId, genericInstance->genericInBuffer,
                                                         genericInstance->reportDescriptorLength, 0,
                                                         genericInstance->reportDescriptorLength);
                }
            }
            genericInstance->runWaitState = kUSB_HostHidRunWaitSetProtocol;
            genericInstance->runState     = kUSB_HostHidRunIdle;
            /* third: set protocol */
            if (USB_HostHidSetProtocol(genericInstance->classHandle, USB_HOST_HID_REQUEST_PROTOCOL_REPORT,
                                       USB_HostHidControlCallback, genericInstance) != kStatus_USB_Success)
            {
                usb_echo("Error in USB_HostHidSetProtocol\r\n");
            }
            break;

        case kUSB_HostHidRunSetProtocolDone: /* 5. start to receive data and send data */
            genericInstance->runWaitState = kUSB_HostHidRunWaitDataReceived;
            genericInstance->runState     = kUSB_HostHidRunIdle;
            if (USB_HostHidRecv(genericInstance->classHandle, genericInstance->genericInBuffer,
                                genericInstance->inMaxPacketSize, USB_HostHidInCallback,
                                genericInstance) != kStatus_USB_Success)
            {
                usb_echo("Error in USB_HostHidRecv\r\n");
            }
            status = USB_HostHidGenericPrepareOutData(genericInstance);
            if (status == kStatus_USB_Success)
            {
                if (USB_HostHidSend(genericInstance->classHandle, genericInstance->genericOutBuffer,
                                    genericInstance->outMaxPacketSize, USB_HostHidOutCallback,
                                    genericInstance) != kStatus_USB_Success)
                {
                    usb_echo("Error in USB_HostHidSend\r\n");
                }
            }
            break;

        case kUSB_HostHidRunDataReceived: /* process received data, receive next data and send next data */
            USB_HostHidGenericProcessBuffer(genericInstance);

            genericInstance->runWaitState = kUSB_HostHidRunWaitDataReceived;
            genericInstance->runState     = kUSB_HostHidRunIdle;
            if (USB_HostHidRecv(genericInstance->classHandle, genericInstance->genericInBuffer,
                                genericInstance->inMaxPacketSize, USB_HostHidInCallback,
                                genericInstance) != kStatus_USB_Success)
            {
                usb_echo("Error in USB_HostHidRecv\r\n");
            }
            status = USB_HostHidGenericPrepareOutData(genericInstance);
            if (status == kStatus_USB_Success)
            {
                if (USB_HostHidSend(genericInstance->classHandle, genericInstance->genericOutBuffer,
                                    genericInstance->outMaxPacketSize, USB_HostHidOutCallback,
                                    genericInstance) != kStatus_USB_Success)
                {
                    usb_echo("Error in USB_HostHidSend\r\n");
                }
            }
            break;

        case kUSB_HostHidRunPrimeDataReceive: /* receive next data and send next data */
            genericInstance->runWaitState = kUSB_HostHidRunWaitDataReceived;
            genericInstance->runState     = kUSB_HostHidRunIdle;
            if (USB_HostHidRecv(genericInstance->classHandle, genericInstance->genericInBuffer,
                                genericInstance->inMaxPacketSize, USB_HostHidInCallback,
                                genericInstance) != kStatus_USB_Success)
            {
                usb_echo("Error in USB_HostHidRecv\r\n");
            }
            status = USB_HostHidGenericPrepareOutData(genericInstance);
            if (status == kStatus_USB_Success)
            {
                if (USB_HostHidSend(genericInstance->classHandle, genericInstance->genericOutBuffer,
                                    genericInstance->outMaxPacketSize, USB_HostHidOutCallback,
                                    genericInstance) != kStatus_USB_Success)
                {
                    usb_echo("Error in USB_HostHidSend\r\n");
                }
            }
            break;

        default:
            break;
    }
}

usb_status_t USB_HostHidGenericEvent(usb_device_handle deviceHandle,
                                     usb_host_configuration_handle configurationHandle,
                                     uint32_t eventCode)
{
    uint32_t pid = 0U;
    uint32_t vid = 0U;
    usb_host_configuration_t *configuration;
    usb_host_interface_t *interface;
    uint32_t infoValue  = 0U;
    usb_status_t status = kStatus_USB_Success;
    uint8_t interfaceIndex;
    uint8_t id;
    usb_host_hid_generic_instance_t *genericInstance;

    switch (eventCode)
    {
        case kUSB_HostEventAttach:
            /* judge whether is configurationHandle supported */
            configuration = (usb_host_configuration_t *)configurationHandle;
            for (interfaceIndex = 0; interfaceIndex < configuration->interfaceCount; ++interfaceIndex)
            {
                interface = &configuration->interfaceList[interfaceIndex];
                USB_HostHelperGetPeripheralInformation(deviceHandle, kUSB_HostGetDevicePID, &pid);
                USB_HostHelperGetPeripheralInformation(deviceHandle, kUSB_HostGetDeviceVID, &vid);
                USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceHubNumber,
                                                         &infoValue);
                usb_echo("attach event: vid=0x%x pid=0x%x hub=%u ", vid, pid, infoValue);
                USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDevicePortNumber,
                                                         &infoValue);
                usb_echo("port=%u ", infoValue);
                USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceHSHubNumber,
                                                         &infoValue);
                usb_echo("hs hub=%u ", infoValue);
                USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceHSHubPort,
                                                         &infoValue);
                usb_echo("hs port=%u ", infoValue);
                USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceLevel, &infoValue);
                usb_echo("level=%u\r\n", infoValue);
                id        = interface->interfaceDesc->bInterfaceClass;
                if (id != USB_HOST_HID_CLASS_CODE)
                {
                    usb_echo("  skip interface %u: class=0x%x not HID\r\n", interfaceIndex, id);
                    continue;
                }
                id = interface->interfaceDesc->bInterfaceSubClass;
                if ((id != USB_HOST_HID_SUBCLASS_CODE_NONE) && (id != USB_HOST_HID_SUBCLASS_CODE_BOOT))
                {
                    usb_echo("  skip interface %u: subclass=0x%x unsupported\r\n", interfaceIndex, id);
                    continue;
                }
                genericInstance = USB_HostHidGenericAllocateInstance();
                if (genericInstance != NULL)
                {
                    /* the interface is supported by the application */
                    uint8_t instanceIndex        = (uint8_t)(genericInstance - g_HostHidGeneric);
                    genericInstance->genericInBuffer  = &s_GenericInBuffer[instanceIndex][0];
                    genericInstance->genericOutBuffer = &s_GenericOutBuffer[instanceIndex][0];
                    genericInstance->deviceHandle     = deviceHandle;
                    genericInstance->interfaceHandle  = interface;
                    genericInstance->configHandle     = configurationHandle;
                    genericInstance->vid              = (uint16_t)vid;
                    genericInstance->pid              = (uint16_t)pid;
                    genericInstance->deviceId         = SPI_DEVICE_ID_INVALID;
                    genericInstance->deviceAnnounced  = false;
                    USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceHubNumber,
                                                             &infoValue);
                    genericInstance->hubNumber = (uint8_t)infoValue;
                    USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDevicePortNumber,
                                                             &infoValue);
                    genericInstance->portNumber = (uint8_t)infoValue;
                    USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceHSHubNumber,
                                                             &infoValue);
                    genericInstance->hsHubNumber = (uint8_t)infoValue;
                    USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceHSHubPort,
                                                             &infoValue);
                    genericInstance->hsHubPort = (uint8_t)infoValue;
                    USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceLevel,
                                                             &infoValue);
                    genericInstance->level = (uint8_t)infoValue;
                    return kStatus_USB_Success;
                }
            }
            status = kStatus_USB_NotSupported;
            break;

        case kUSB_HostEventNotSupported:
            break;

        case kUSB_HostEventEnumerationDone:
            genericInstance = USB_HostHidGenericFindInstanceByConfig(configurationHandle);
            if (genericInstance != NULL)
            {
                if ((genericInstance->deviceHandle != NULL) && (genericInstance->interfaceHandle != NULL))
                {
                    /* the device enumeration is done */
                    if (genericInstance->deviceState == kStatus_DEV_Idle)
                    {
                        genericInstance->deviceState = kStatus_DEV_Attached;

                        USB_HostHelperGetPeripheralInformation(deviceHandle, kUSB_HostGetDevicePID, &infoValue);
                        usb_echo("hid generic attached:pid=0x%x", infoValue);
                        USB_HostHelperGetPeripheralInformation(deviceHandle, kUSB_HostGetDeviceVID, &infoValue);
                        usb_echo("vid=0x%x ", infoValue);
                        USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceHubNumber,
                                                                 &infoValue);
                        usb_echo("hub=%u ", infoValue);
                        USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDevicePortNumber,
                                                                 &infoValue);
                        usb_echo("port=%u ", infoValue);
                        USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceHSHubNumber,
                                                                 &infoValue);
                        usb_echo("hs hub=%u ", infoValue);
                        USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceHSHubPort,
                                                                 &infoValue);
                        usb_echo("hs port=%u ", infoValue);
                        USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceLevel,
                                                                 &infoValue);
                        usb_echo("level=%u ", infoValue);
                        USB_HostHelperGetPeripheralInformation(deviceHandle, kUSB_HostGetDeviceAddress, &infoValue);
                        usb_echo("address=%d\r\n", infoValue);
                    }
                    else
                    {
                        usb_echo("not idle generic instance\r\n");
                        status = kStatus_USB_Error;
                    }
                }
            }
            break;

        case kUSB_HostEventDetach:
            genericInstance = USB_HostHidGenericFindInstanceByConfig(configurationHandle);
            if (genericInstance == NULL)
            {
                genericInstance = USB_HostHidGenericFindInstanceByDevice(deviceHandle);
            }
            if (genericInstance != NULL)
            {
                /* the device is detached */
                genericInstance->configHandle = NULL;
                if (genericInstance->deviceState != kStatus_DEV_Idle)
                {
                    if (genericInstance->deviceAnnounced && (genericInstance->deviceId != SPI_DEVICE_ID_INVALID))
                    {
                        (void)SPI_BridgeRemoveDevice(genericInstance->deviceId);
                    }
                    genericInstance->deviceAnnounced = false;
                    genericInstance->deviceId        = SPI_DEVICE_ID_INVALID;
                    genericInstance->deviceState     = kStatus_DEV_Detached;
                }
            }
            break;

        default:
            break;
    }
    return status;
}
