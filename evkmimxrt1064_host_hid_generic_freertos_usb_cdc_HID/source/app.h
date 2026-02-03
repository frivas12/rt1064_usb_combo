/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 - 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef __APP_H__
#define __APP_H__
/*
 * USB combo configuration:
 * - APP_USB_ENABLE_CDC / APP_USB_ENABLE_HID enable each class independently.
 * - APP_USB_CDC_EHCI_INSTANCE / APP_USB_HID_EHCI_INSTANCE select the connector:
 *   0 = USB_OTG1 (J9), 1 = USB_OTG2 (J10).
 */
#ifndef APP_USB_ENABLE_CDC
#define APP_USB_ENABLE_CDC 1U
#endif
#ifndef APP_USB_ENABLE_HID
#define APP_USB_ENABLE_HID 1U
#endif
#ifndef APP_USB_CDC_EHCI_INSTANCE
#define APP_USB_CDC_EHCI_INSTANCE 0U
#endif
#ifndef APP_USB_HID_EHCI_INSTANCE
#define APP_USB_HID_EHCI_INSTANCE 1U
#endif

#ifndef USB_DEVICE_CONFIG_CDC_ACM
#define USB_DEVICE_CONFIG_CDC_ACM APP_USB_ENABLE_CDC
#endif
#ifndef USB_HOST_CONFIG_HID
#define USB_HOST_CONFIG_HID APP_USB_ENABLE_HID
#endif
#ifndef USB_CDC_EHCI_INSTANCE
#define USB_CDC_EHCI_INSTANCE APP_USB_CDC_EHCI_INSTANCE
#endif
#ifndef USB_HOST_EHCI_INSTANCE
#define USB_HOST_EHCI_INSTANCE APP_USB_HID_EHCI_INSTANCE
#endif

#include "usb_host_config.h"
#include "usb_host.h"
#include "fsl_device_registers.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* @TEST_ANCHOR */

#if ((defined USB_HOST_CONFIG_KHCI) && (USB_HOST_CONFIG_KHCI))
#ifndef CONTROLLER_ID
#define CONTROLLER_ID kUSB_ControllerKhci0
#endif
#endif /* USB_HOST_CONFIG_KHCI */
#if ((defined USB_HOST_CONFIG_EHCI) && (USB_HOST_CONFIG_EHCI))
#ifndef CONTROLLER_ID
#if (USB_HOST_EHCI_INSTANCE == 0U)
#define CONTROLLER_ID kUSB_ControllerEhci0
#else
#define CONTROLLER_ID kUSB_ControllerEhci1
#endif
#endif
#endif /* USB_HOST_CONFIG_EHCI */
#if ((defined USB_HOST_CONFIG_OHCI) && (USB_HOST_CONFIG_OHCI))
#ifndef CONTROLLER_ID
#define CONTROLLER_ID kUSB_ControllerOhci0
#endif
#endif /* USB_HOST_CONFIG_OHCI */
#if ((defined USB_HOST_CONFIG_IP3516HS) && (USB_HOST_CONFIG_IP3516HS))
#ifndef CONTROLLER_ID
#define CONTROLLER_ID kUSB_ControllerIp3516Hs0
#endif
#endif /* USB_HOST_CONFIG_IP3516HS */

#if defined(__GIC_PRIO_BITS)
#define USB_HOST_INTERRUPT_PRIORITY (25U)
#elif defined(__NVIC_PRIO_BITS) && (__NVIC_PRIO_BITS >= 3)
#define USB_HOST_INTERRUPT_PRIORITY (6U)
#else
#define USB_HOST_INTERRUPT_PRIORITY (3U)
#endif

#ifndef APP_SPI_PIN_TOGGLE_TEST
#define APP_SPI_PIN_TOGGLE_TEST 0U
#endif

#define APP_USB_TASK_PRIORITY (tskIDLE_PRIORITY + 5)
#define APP_MAIN_TASK_PRIORITY (APP_USB_TASK_PRIORITY - 1)

/*! @brief host app device attach/detach status */
typedef enum _usb_host_app_state
{
    kStatus_DEV_Idle = 0, /*!< there is no device attach/detach */
    kStatus_DEV_Attached, /*!< device is attached */
    kStatus_DEV_Detached, /*!< device is detached */
} usb_host_app_state_t;

#endif /* __APP_H__ */
