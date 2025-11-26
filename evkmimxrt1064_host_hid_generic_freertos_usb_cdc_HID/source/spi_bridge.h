/*
 * SPI bridge API (SBIBridge.h)
 *
 * Describes the SPI Bridge Interface (SBI) register map mirrored by the
 * SPI bridge task so an external coprocessor can monitor USB HID activity
 * without implementing a USB stack. Callers allocate logical device slots,
 * publish IN reports, and retrieve OUT reports that arrive over SPI.
 *
 * The layout mimics a small memory-mapped peripheral: a hub-status block
 * followed by per-device IN/OUT blocks. Each block includes a one-byte
 * header with DIRTY/TYPE/LEN fields, a payload, and a CRC. The SPI slave task
 * streams those blocks whenever the host clocks the bus, keeping the
 * coprocessor in sync with real-time HID traffic.
*/
#ifndef _SPI_BRIDGE_H_
#define _SPI_BRIDGE_H_

#include <stdbool.h>

#include "fsl_common.h"

#ifndef SPI_BRIDGE_ENABLE_DEBUG
#define SPI_BRIDGE_ENABLE_DEBUG (1U)
#endif

/* When enabled, every register-map update is logged at the moment it is applied. */
#ifndef SPI_BRIDGE_ENABLE_STATE_TRACE
#define SPI_BRIDGE_ENABLE_STATE_TRACE (0U)
#endif

#if !SPI_BRIDGE_ENABLE_DEBUG
#warning "SPI bridge debug is disabled in this build"
#endif

#define SPI_BRIDGE_STRINGIFY(x) #x
#define SPI_BRIDGE_TOSTRING(x) SPI_BRIDGE_STRINGIFY(x)
#define SPI_BRIDGE_ENABLE_DEBUG_STRING SPI_BRIDGE_TOSTRING(SPI_BRIDGE_ENABLE_DEBUG)

#if SPI_BRIDGE_ENABLE_DEBUG
#define SPI_BRIDGE_LOG(...) usb_echo(__VA_ARGS__)
#else
#define SPI_BRIDGE_LOG(...) ((void)0)
#endif

#ifndef SPI_BRIDGE_MAX_DEVICES
#define SPI_BRIDGE_MAX_DEVICES (5U)
#endif

#define SPI_DEVICE_ID_INVALID (0xFFU)

/* New 1-byte header bitfields shared by hub, IN, and OUT blocks. */
#define SPI_BRIDGE_HEADER_DIRTY_MASK  (0x01U)
#define SPI_BRIDGE_HEADER_TYPE_MASK   (0x02U)
#define SPI_BRIDGE_HEADER_LENGTH_MASK (0xFCU)
#define SPI_BRIDGE_HEADER_LENGTH_SHIFT (2U)

#define SPI_BRIDGE_MAX_PAYLOAD_LENGTH (63U)
#define SPI_BRIDGE_BLOCK_SIZE         (1U + SPI_BRIDGE_MAX_PAYLOAD_LENGTH + 2U)

#define SPI_BRIDGE_CRC_POLY (0x1021U)
#define SPI_BRIDGE_CRC_INIT (0xFFFFU)

typedef struct _spi_bridge_block
{
    uint8_t header;
    uint8_t payload[SPI_BRIDGE_MAX_PAYLOAD_LENGTH];
    uint16_t crc;
} spi_bridge_block_t;

/*!
 * @brief Initializes the SPI slave used to stream HID and CDC information to the V70.
 *
 * This call sets up LPSPI1 for 8-bit frames in slave mode and creates the
 * backing queue used by @ref SPI_BridgeTask. Invoke this before starting the
 * scheduler so queued messages are not lost.
 */
status_t SPI_BridgeInit(void);

/*!
 * @brief RTOS task that mirrors the register map over SPI slave.
 */
void SPI_BridgeTask(void *param);

/*!
 * @brief Allocates a logical device slot and updates hub status.
 */
status_t SPI_BridgeAllocDevice(uint8_t *deviceIdOut);

/*!
 * @brief Frees a logical device slot and marks hub status dirty.
 */
status_t SPI_BridgeRemoveDevice(uint8_t deviceId);

/*!
 * @brief Publishes a HID report descriptor (TYPE=1) to the device IN buffer.
 */
status_t SPI_BridgeSendReportDescriptor(uint8_t deviceId, const uint8_t *descriptor, uint16_t length);

/*!
 * @brief Publishes HID report data into the IN buffer (inDirection=true) or ignores OUT requests.
 */
status_t SPI_BridgeSendReport(uint8_t deviceId, bool inDirection, uint8_t reportId, const uint8_t *data, uint16_t length);

/*!
 * @brief Reads OUT buffer content if DIRTY is set and CRC is valid.
 */
bool SPI_BridgeGetOutReport(uint8_t deviceId, uint8_t *typeOut, uint8_t *payloadOut, uint8_t *lengthOut);

/*!
 * @brief Clears the DIRTY flag for an OUT buffer after the payload is consumed.
 */
status_t SPI_BridgeClearOutReport(uint8_t deviceId);

#endif /* _SPI_BRIDGE_H_ */
