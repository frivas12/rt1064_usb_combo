#ifndef _SPI_BRIDGE_H_
#define _SPI_BRIDGE_H_

#include <stdbool.h>

#include "fsl_common.h"
#include "FreeRTOS.h"
#include "queue.h"

#ifndef SPI_BRIDGE_MAX_PAYLOAD
#define SPI_BRIDGE_MAX_PAYLOAD (512U)
#endif

#ifndef SPI_BRIDGE_QUEUE_DEPTH
#define SPI_BRIDGE_QUEUE_DEPTH (6U)
#endif

/*!
 * @brief Message type identifiers for frames emitted to the Quark over SPI.
 */
typedef enum _spi_bridge_message_type
{
    kSpiBridgeMessageAttach = 0x01U,           /*!< HID device attached. */
    kSpiBridgeMessageDetach = 0x02U,           /*!< HID device detached. */
    kSpiBridgeMessageReportDescriptor = 0x03U, /*!< HID report descriptor payload. */
    kSpiBridgeMessageReportIn = 0x04U,         /*!< HID interrupt IN payload. */
    kSpiBridgeMessageReportOut = 0x05U         /*!< HID interrupt OUT payload. */
} spi_bridge_message_type_t;

/*!
 * @brief Header prepended to every SPI frame.
 */
typedef struct _spi_bridge_header
{
    uint8_t messageType; /*!< One of @ref spi_bridge_message_type_t. */
    uint8_t source;      /*!< Direction or endpoint identifier. */
    uint16_t length;     /*!< Payload length in bytes. */
} spi_bridge_header_t;

/*!
 * @brief Initializes the SPI slave used to stream HID information to Quark.
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
 * @brief Queues a device attach or detach indication.
 */
status_t SPI_BridgeSendDeviceEvent(bool attached, uint16_t vid, uint16_t pid);

/*!
 * @brief Queues a report descriptor payload for the active HID interface.
 */
status_t SPI_BridgeSendReportDescriptor(const uint8_t *descriptor, uint16_t length);

/*!
 * @brief Queues HID report data observed on the interrupt endpoints.
 */
status_t SPI_BridgeSendReport(bool inDirection, const uint8_t *data, uint16_t length);

#endif /* _SPI_BRIDGE_H_ */
