/*
 * Minimal SPI bridge API
 *
 * Provides a byte-level SPI test between the RT1064 (slave) and SAMV70 (master).
 * The slave returns the last received byte + 1 on the next SPI byte.
 */
#ifndef _SPI_BRIDGE_H_
#define _SPI_BRIDGE_H_

#include <stdbool.h>
#include <stdint.h>

#include "fsl_common.h"

#ifndef SPI_BRIDGE_MAX_DEVICES
#define SPI_BRIDGE_MAX_DEVICES (4U)
#define SPI_BRIDGE_CDC_ENDPOINT_INDEX (SPI_BRIDGE_MAX_DEVICES + 1U)
#endif

#define SPI_BRIDGE_MAX_PAYLOAD_LENGTH (63U)

status_t SPI_BridgeInit(void);
void SPI_BridgeTask(void *param);

status_t SPI_BridgeAllocDevice(uint8_t *deviceIdOut);
status_t SPI_BridgeRemoveDevice(uint8_t deviceId);
status_t SPI_BridgeSendReportDescriptor(uint8_t deviceId, const uint8_t *descriptor, uint16_t length);
status_t SPI_BridgeSendReport(uint8_t deviceId, bool inDirection, uint8_t reportId, const uint8_t *data, uint16_t length);
status_t SPI_BridgeSendCdcIn(const uint8_t *data, uint8_t length);
status_t SPI_BridgeSendCdcDescriptor(const uint8_t *descriptor, uint8_t length);
bool SPI_BridgeGetOutReport(uint8_t deviceId, uint8_t *typeOut, uint8_t *payloadOut, uint8_t *lengthOut);
status_t SPI_BridgeClearOutReport(uint8_t deviceId);
bool SPI_BridgeGetCdcOut(uint8_t *typeOut, uint8_t *payloadOut, uint8_t *lengthOut);
status_t SPI_BridgeClearCdcOut(void);
void SPI_BridgeSetStateTraceEnabled(bool enabled);
bool SPI_BridgeStateTraceEnabled(void);
void SPI_BridgeEnableFixedCdcResponse(void);

#endif /* _SPI_BRIDGE_H_ */
