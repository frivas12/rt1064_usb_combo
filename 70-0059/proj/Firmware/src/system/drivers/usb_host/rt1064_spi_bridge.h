// rt1064_spi_bridge.h

#ifndef SRC_SYSTEM_DRIVERS_USB_HOST_RT1064_SPI_BRIDGE_H_
#define SRC_SYSTEM_DRIVERS_USB_HOST_RT1064_SPI_BRIDGE_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern volatile bool last_rx_good;
extern volatile uint32_t good_count;
extern volatile uint32_t bad_count;

extern uint8_t last_rx[64];
extern uint8_t last_tx[64];

/**
 * @brief Initialize the RT1064 SPI bridge task when the RT1064 USB host is present.
 */
void rt1064_spi_bridge_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_USB_HOST_RT1064_SPI_BRIDGE_H_ */
