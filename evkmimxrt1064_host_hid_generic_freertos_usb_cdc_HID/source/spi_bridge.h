#ifndef _SPI_BRIDGE_H_
#define _SPI_BRIDGE_H_

#include <stdbool.h>

#include "fsl_common.h"
#include "FreeRTOS.h"
#include "queue.h"

#ifndef SPI_BRIDGE_ENABLE_DEBUG
#define SPI_BRIDGE_ENABLE_DEBUG (0U)
#endif

#if SPI_BRIDGE_ENABLE_DEBUG
#define SPI_BRIDGE_LOG(...) usb_echo("[USB Host] " __VA_ARGS__)
#else
#define SPI_BRIDGE_LOG(...) ((void)0)
#endif

#ifndef SPI_BRIDGE_MAX_PAYLOAD
#define SPI_BRIDGE_MAX_PAYLOAD (512U)
#endif

#ifndef SPI_BRIDGE_QUEUE_DEPTH
#define SPI_BRIDGE_QUEUE_DEPTH (6U)
#endif

#ifndef SPI_BRIDGE_MAX_DEVICES
#define SPI_BRIDGE_MAX_DEVICES (5U)
#endif

#define SPI_BRIDGE_SYNC_BYTE   (0xA5U)
#define SPI_BRIDGE_VERSION     (0x01U)
#define SPI_BRIDGE_CRC_POLY    (0x1021U)
#define SPI_BRIDGE_CRC_INIT    (0xFFFFU)

/*! @brief Message type identifiers shared between the RT1064 and V70. */
typedef enum _spi_bridge_message_type
{
    kSpiBridgeMessageDeviceAdd        = 0x01U, /*!< RT1064 -> V70 device announcement. */
    kSpiBridgeMessageDeviceRemove     = 0x02U, /*!< RT1064 -> V70 device removal. */
    kSpiBridgeMessageReportDescriptor = 0x03U, /*!< HID report descriptor payload. */
    kSpiBridgeMessageReportIn         = 0x04U, /*!< HID interrupt IN payload. */
    kSpiBridgeMessageReportOut        = 0x05U, /*!< HID interrupt OUT payload. */
    kSpiBridgeMessagePing             = 0x10U, /*!< Link keepalive. */
    kSpiBridgeMessagePong             = 0x11U, /*!< Link keepalive response. */
    kSpiBridgeMessageCdcDataIn        = 0x20U, /*!< CDC RX payload to the V70. */
    kSpiBridgeMessageCdcDataOut       = 0x21U, /*!< CDC TX payload from the V70. */
    kSpiBridgeMessageCdcControl       = 0x22U, /*!< CDC line coding/control exchange. */
    kSpiBridgeMessageError            = 0x7FU  /*!< Error reporting. */
} spi_bridge_message_type_t;

/*! @brief Device type identifiers. */
typedef enum _spi_bridge_device_type
{
    kSpiBridgeDeviceGenericHid = 0U,
    kSpiBridgeDeviceJoystick   = 1U,
    kSpiBridgeDeviceKeyboard   = 2U,
    kSpiBridgeDeviceMouse      = 3U,
    kSpiBridgeDeviceCdc        = 4U
} spi_bridge_device_type_t;

/*! @brief Header prepended to every SPI frame. */
typedef struct __attribute__((packed)) _spi_bridge_header
{
    uint8_t sync;
    uint8_t version;
    uint8_t msgType;
    uint8_t deviceId;
    uint16_t length;
    uint16_t seq;
    uint16_t crc;
} spi_bridge_header_t;

/*! @brief DEVICE_ADD payload. */
typedef struct __attribute__((packed)) _spi_bridge_device_add
{
    uint16_t vid;
    uint16_t pid;
    uint8_t interfaceNumber;
    uint8_t hasOutEndpoint;
    uint8_t deviceType;
    uint16_t reportDescLen;
    uint8_t hubNumber;
    uint8_t portNumber;
    uint8_t hsHubNumber;
    uint8_t hsHubPort;
    uint8_t level;
} spi_bridge_device_add_t;

/*! @brief REPORT_DESCRIPTOR payload. */
typedef struct __attribute__((packed)) _spi_bridge_report_descriptor
{
    uint16_t totalLength;
    uint16_t offset;
    uint16_t chunkLength;
    uint8_t data[];
} spi_bridge_report_descriptor_t;

/*! @brief REPORT_IN/REPORT_OUT payload. */
typedef struct __attribute__((packed)) _spi_bridge_report
{
    uint8_t reportId;
    uint16_t reportLength;
    uint8_t data[];
} spi_bridge_report_t;

/*! @brief CDC data payload. */
typedef struct __attribute__((packed)) _spi_bridge_cdc_data
{
    uint16_t dataLength;
    uint8_t data[];
} spi_bridge_cdc_data_t;

/*! @brief CDC control payload. */
typedef struct __attribute__((packed)) _spi_bridge_cdc_control
{
    uint32_t baudrate;
    uint8_t parity;
    uint8_t stopBits;
    uint8_t dataBits;
    uint8_t dtr;
    uint8_t rts;
} spi_bridge_cdc_control_t;

/*!
 * @brief Initializes the SPI slave used to stream HID and CDC information to the V70.
 *
 * This call sets up LPSPI1 for 8-bit frames in slave mode and creates the
 * backing queue used by @ref SPI_BridgeTask. Invoke this before starting the
 * scheduler so queued messages are not lost.
 */
status_t SPI_BridgeInit(void);

/*!
 * @brief RTOS task that drains the bridge queue and pushes frames to SPI.
 */
void SPI_BridgeTask(void *param);

/*!
 * @brief Allocates a logical device ID and announces it over SPI.
 */
status_t SPI_BridgeAllocDevice(spi_bridge_device_type_t deviceType,
                               uint16_t vid,
                               uint16_t pid,
                               uint8_t interfaceNumber,
                               bool hasOutEndpoint,
                               uint16_t reportDescLen,
                               uint8_t hubNumber,
                               uint8_t portNumber,
                               uint8_t hsHubNumber,
                               uint8_t hsHubPort,
                               uint8_t level,
                               uint8_t *deviceIdOut);

/*!
 * @brief Announces device removal and frees the logical device ID.
 */
status_t SPI_BridgeRemoveDevice(uint8_t deviceId);

/*!
 * @brief Queues a report descriptor chunk for a HID interface.
 */
status_t SPI_BridgeSendReportDescriptor(uint8_t deviceId,
                                        const uint8_t *descriptor,
                                        uint16_t totalLength,
                                        uint16_t offset,
                                        uint16_t chunkLength);

/*!
 * @brief Queues HID report data observed on the interrupt endpoints.
 */
status_t SPI_BridgeSendReport(uint8_t deviceId, bool inDirection, uint8_t reportId, const uint8_t *data, uint16_t length);

/*!
 * @brief Queues CDC payload data on the SPI link.
 */
status_t SPI_BridgeSendCdcData(uint8_t deviceId, bool inDirection, const uint8_t *data, uint16_t length);

#endif /* _SPI_BRIDGE_H_ */
