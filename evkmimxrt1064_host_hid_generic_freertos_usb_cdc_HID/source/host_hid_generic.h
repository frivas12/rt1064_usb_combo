/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016, 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HOST_HID_GENERIC_H_
#define _HOST_HID_GENERIC_H_

#include <stdbool.h>

#include "spi_bridge.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief Set to 0 to compile out all USB HID log messages. */
#ifndef HID_GENERIC_ENABLE_LOG
#define HID_GENERIC_ENABLE_LOG (1U)
#endif

#if HID_GENERIC_ENABLE_LOG
#define HID_GENERIC_LOG(...) usb_echo("[USB Host] " __VA_ARGS__)
#else
#define HID_GENERIC_LOG(...) ((void)0)
#endif

/*! @brief buffer for receiving report descriptor and data */
#define HID_GENERIC_IN_BUFFER_SIZE (100U)
/*! @brief buffer for sending data */
#define HID_GENERIC_OUT_BUFFER_SIZE (8U)
/*! @brief maximum number of HID instances tracked by the bridge */
#define HID_GENERIC_MAX_DEVICES (SPI_BRIDGE_MAX_DEVICES)

/*! @brief host app run status */
typedef enum _usb_host_hid_generic_run_state
{
    kUSB_HostHidRunIdle = 0,                /*!< idle */
    kUSB_HostHidRunSetInterface,            /*!< execute set interface code */
    kUSB_HostHidRunWaitSetInterface,        /*!< wait set interface done */
    kUSB_HostHidRunSetInterfaceDone,        /*!< set interface is done, execute next step */
    kUSB_HostHidRunWaitSetIdle,             /*!< wait set idle done */
    kUSB_HostHidRunSetIdleDone,             /*!< set idle is done, execute next step */
    kUSB_HostHidRunWaitGetReportDescriptor, /*!< wait get report descriptor done */
    kUSB_HostHidRunGetReportDescriptorDone, /*!< get report descriptor is done, execute next step */
    kUSB_HostHidRunWaitSetProtocol,         /*!< wait set protocol done */
    kUSB_HostHidRunSetProtocolDone,         /*!< set protocol is done, execute next step */
    kUSB_HostHidRunWaitDataReceived,        /*!< wait interrupt in data */
    kUSB_HostHidRunDataReceived,            /*!< interrupt in data received */
    kUSB_HostHidRunPrimeDataReceive,        /*!< prime interrupt in receive */
} usb_host_hid_generic_run_state_t;

/*! @brief USB host hid generic instance structure */
typedef struct _usb_host_hid_generic_instance
{
    usb_host_configuration_handle configHandle; /*!< the hid generic's configuration handle */
    usb_device_handle deviceHandle;             /*!< the hid generic's device handle */
    usb_host_class_handle classHandle;          /*!< the hid generic's class handle */
    usb_host_interface_handle interfaceHandle;  /*!< the hid generic's interface handle */
    uint8_t deviceState;                        /*!< device attach/detach status */
    uint8_t prevState;                          /*!< device attach/detach previous status */
    uint8_t runState;                           /*!< hid generic application run status */
    uint8_t
        runWaitState; /*!< hid generic application wait status, go to next run status when the wait status success */
    uint16_t inMaxPacketSize;  /*!< Interrupt in max packet size */
    uint16_t outMaxPacketSize; /*!< Interrupt out max packet size */
    uint8_t *genericInBuffer;  /*!< use to receive report descriptor and data */
    uint8_t *genericOutBuffer; /*!< use to send data */
    uint16_t sendIndex;        /*!< data sending position */
    uint16_t reportDescriptorLength; /*!< length of the active HID report descriptor */
    uint16_t lastInDataLength;       /*!< most recent interrupt IN payload size */
    uint16_t lastOutDataLength;      /*!< most recent interrupt OUT payload size */
    uint16_t vid;                    /*!< attached device vendor ID */
    uint16_t pid;                    /*!< attached device product ID */
    uint8_t hubNumber;               /*!< parent hub address (0 = root hub) */
    uint8_t portNumber;              /*!< parent hub port number */
    uint8_t hsHubNumber;             /*!< upstream high-speed hub address */
    uint8_t hsHubPort;               /*!< upstream high-speed hub port */
    uint8_t level;                   /*!< hub depth level */
    uint8_t deviceId;                /*!< logical device ID for SPI bridge */
    bool deviceAnnounced;            /*!< true once DEVICE_ADD has been sent */
} usb_host_hid_generic_instance_t;

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @brief host hid generic task function.
 *
 * This function implements the host hid generic action, it is used to create task.
 *
 * @param param   the host hid generic instance pointer.
 */
extern void USB_HostHidGenericTask(void *param);

/*!
 * @brief host hid generic callback function.
 *
 * This function should be called in the host callback function.
 *
 * @param deviceHandle         device handle.
 * @param configurationHandle  attached device's configuration descriptor information.
 * @param eventCode            callback event code, please reference to enumeration host_event_t.
 *
 * @retval kStatus_USB_Success              The host is initialized successfully.
 * @retval kStatus_USB_NotSupported         The configuration don't contain hid generic interface.
 */
extern usb_status_t USB_HostHidGenericEvent(usb_device_handle deviceHandle,
                                            usb_host_configuration_handle configurationHandle,
                                            uint32_t eventCode);

#endif /* _HOST_HID_GENERIC_H_ */
