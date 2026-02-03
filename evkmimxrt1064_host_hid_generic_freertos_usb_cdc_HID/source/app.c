/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 - 2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "usb_host_config.h"
#include "usb_host.h"
#include "fsl_device_registers.h"
#include "usb_host_hid.h"
#include "host_hid_generic.h"
#include "fsl_common.h"
#include "board.h"
#include "fsl_gpio.h"
#include "fsl_iomuxc.h"
#include "spi_bridge.h"
#include "virtual_com.h"
#include "FreeRTOS.h"
#include "task.h"
#if (defined(FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT > 0U))
#include "fsl_sysmpu.h"
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */
#include "app.h"

#if ((!USB_HOST_CONFIG_KHCI) && (!USB_HOST_CONFIG_EHCI) && (!USB_HOST_CONFIG_OHCI) && (!USB_HOST_CONFIG_IP3516HS))
#error Please enable USB_HOST_CONFIG_KHCI, USB_HOST_CONFIG_EHCI, USB_HOST_CONFIG_OHCI, or USB_HOST_CONFIG_IP3516HS in file usb_host_config.
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief host callback function.
 *
 * device attach/detach callback function.
 *
 * @param deviceHandle         device handle.
 * @param configurationHandle  attached device's configuration descriptor information.
 * @param eventCode            callback event code, please reference to enumeration host_event_t.
 *
 * @retval kStatus_USB_Success              The host is initialized successfully.
 * @retval kStatus_USB_NotSupported         The application don't support the configuration.
 */
static usb_status_t USB_HostEvent(usb_device_handle deviceHandle,
                                  usb_host_configuration_handle configurationHandle,
                                  uint32_t eventCode);

/*!
 * @brief app initialization.
 */
static void USB_HostApplicationInit(void);

/*!
 * @brief host freertos task function.
 *
 * @param g_HostHandle   host handle
 */
static void USB_HostTask(void *param);

/*!
 * @brief host mouse freertos task function.
 *
 * @param param   the host mouse instance pointer.
 */
static void USB_HostApplicationTask(void *param);

#if APP_SPI_PIN_TOGGLE_TEST
static void SPI_PinToggleInit(void);
static void SPI_PinToggleTask(void *param);
#endif

extern void USB_HostClockInit(void);
extern void USB_HostIsrEnable(void);
extern void USB_HostTaskFn(void *param);
void BOARD_InitHardware(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief USB host generic instance global variable */
extern usb_host_hid_generic_instance_t g_HostHidGeneric[HID_GENERIC_MAX_DEVICES];
usb_host_handle g_HostHandle;

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief USB isr function.
 */

static usb_status_t USB_HostEvent(usb_device_handle deviceHandle,
                                  usb_host_configuration_handle configurationHandle,
                                  uint32_t eventCode)
{
    usb_status_t status = kStatus_USB_Success;

    switch (eventCode & 0x0000FFFFU)
    {
        case kUSB_HostEventAttach:
            status = USB_HostHidGenericEvent(deviceHandle, configurationHandle, eventCode);
            break;

        case kUSB_HostEventNotSupported:
            usb_echo("device not supported.\r\n");
            break;

        case kUSB_HostEventEnumerationDone:
            status = USB_HostHidGenericEvent(deviceHandle, configurationHandle, eventCode);
            break;

        case kUSB_HostEventDetach:
            status = USB_HostHidGenericEvent(deviceHandle, configurationHandle, eventCode);
            break;

        case kUSB_HostEventEnumerationFail:
            usb_echo("enumeration failed\r\n");
            break;

        default:
            break;
    }
    return status;
}

static void USB_HostApplicationInit(void)
{
    usb_status_t status = kStatus_USB_Success;

    USB_HostClockInit();

#if ((defined FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT))
    SYSMPU_Enable(SYSMPU, 0);
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */

    status = USB_HostInit(CONTROLLER_ID, &g_HostHandle, USB_HostEvent);
    if (status != kStatus_USB_Success)
    {
        usb_echo("host init error\r\n");
        return;
    }
    USB_HostIsrEnable();

    usb_echo("host init done\r\n");
}

static void USB_HostTask(void *param)
{
    while (1)
    {
        USB_HostTaskFn(param);
    }
}

static void USB_HostApplicationTask(void *param)
{
    usb_host_hid_generic_instance_t *instances = (usb_host_hid_generic_instance_t *)param;

    while (1)
    {
        for (uint8_t index = 0U; index < HID_GENERIC_MAX_DEVICES; ++index)
        {
            USB_HostHidGenericTask(&instances[index]);
        }
    }
}

#if APP_SPI_PIN_TOGGLE_TEST
#define SPI_PIN_SCK_GPIO GPIO3
#define SPI_PIN_SCK_PIN 12U
#define SPI_PIN_CS_GPIO GPIO3
#define SPI_PIN_CS_PIN 13U
#define SPI_PIN_MOSI_GPIO GPIO3
#define SPI_PIN_MOSI_PIN 14U
#define SPI_PIN_MISO_GPIO GPIO3
#define SPI_PIN_MISO_PIN 15U

static void SPI_PinToggleInit(void)
{
    gpio_pin_config_t config = {kGPIO_DigitalOutput, 0U, kGPIO_NoIntmode};

    IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_00_GPIO3_IO12, 0U);
    IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_01_GPIO3_IO13, 0U);
    IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_02_GPIO3_IO14, 0U);
    IOMUXC_SetPinMux(IOMUXC_GPIO_SD_B0_03_GPIO3_IO15, 0U);

    IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_00_GPIO3_IO12, 0x10B0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_01_GPIO3_IO13, 0x10B0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_02_GPIO3_IO14, 0x10B0U);
    IOMUXC_SetPinConfig(IOMUXC_GPIO_SD_B0_03_GPIO3_IO15, 0x10B0U);

    GPIO_PinInit(SPI_PIN_SCK_GPIO, SPI_PIN_SCK_PIN, &config);
    GPIO_PinInit(SPI_PIN_CS_GPIO, SPI_PIN_CS_PIN, &config);
    GPIO_PinInit(SPI_PIN_MOSI_GPIO, SPI_PIN_MOSI_PIN, &config);
    GPIO_PinInit(SPI_PIN_MISO_GPIO, SPI_PIN_MISO_PIN, &config);
}

static void SPI_PinToggleTask(void *param)
{
    (void)param;

    for (;;)
    {
        GPIO_PinWrite(SPI_PIN_CS_GPIO, SPI_PIN_CS_PIN, 1U);
        vTaskDelay(pdMS_TO_TICKS(100U));
        GPIO_PinWrite(SPI_PIN_CS_GPIO, SPI_PIN_CS_PIN, 0U);
        vTaskDelay(pdMS_TO_TICKS(100U));

        GPIO_PinWrite(SPI_PIN_SCK_GPIO, SPI_PIN_SCK_PIN, 1U);
        vTaskDelay(pdMS_TO_TICKS(100U));
        GPIO_PinWrite(SPI_PIN_SCK_GPIO, SPI_PIN_SCK_PIN, 0U);
        vTaskDelay(pdMS_TO_TICKS(100U));

        GPIO_PinWrite(SPI_PIN_MOSI_GPIO, SPI_PIN_MOSI_PIN, 1U);
        vTaskDelay(pdMS_TO_TICKS(100U));
        GPIO_PinWrite(SPI_PIN_MOSI_GPIO, SPI_PIN_MOSI_PIN, 0U);
        vTaskDelay(pdMS_TO_TICKS(100U));

        GPIO_PinWrite(SPI_PIN_MISO_GPIO, SPI_PIN_MISO_PIN, 1U);
        vTaskDelay(pdMS_TO_TICKS(100U));
        GPIO_PinWrite(SPI_PIN_MISO_GPIO, SPI_PIN_MISO_PIN, 0U);
        vTaskDelay(pdMS_TO_TICKS(100U));
    }
}
#endif

int main(void)
{
    BOARD_InitHardware();

#if APP_SPI_PIN_TOGGLE_TEST
    SPI_PinToggleInit();
#endif

    USB_HostApplicationInit();

    if (USB_DeviceCdcVcomInit() != kStatus_USB_Success)
    {
        usb_echo("usb device cdc vcom init error\r\n");
    }

#if !APP_SPI_PIN_TOGGLE_TEST
    if (SPI_BridgeInit() != kStatus_Success)
    {
        usb_echo("spi bridge init error\r\n");
    }
#endif

    if (xTaskCreate(USB_HostTask, "usb host task", 2000L / sizeof(portSTACK_TYPE), g_HostHandle, APP_USB_TASK_PRIORITY, NULL) != pdPASS)
    {
        usb_echo("create host task error\r\n");
    }
    if (xTaskCreate(USB_HostApplicationTask, "app task", 2000L / sizeof(portSTACK_TYPE), &g_HostHidGeneric, APP_MAIN_TASK_PRIORITY, NULL) !=
        pdPASS)
    {
        usb_echo("create mouse task error\r\n");
    }

#if APP_SPI_PIN_TOGGLE_TEST
    if (xTaskCreate(SPI_PinToggleTask, "spi pin toggle", 1024L / sizeof(portSTACK_TYPE), NULL, APP_MAIN_TASK_PRIORITY, NULL) != pdPASS)
    {
        usb_echo("create spi pin toggle task error\r\n");
    }
#else
    if (xTaskCreate(SPI_BridgeTask, "spi bridge", 2048L / sizeof(portSTACK_TYPE), NULL, APP_MAIN_TASK_PRIORITY, NULL) != pdPASS)
    {
        usb_echo("create spi bridge task error\r\n");
    }
#endif

    vTaskStartScheduler();

    while (1)
    {
        ;
    }
}
