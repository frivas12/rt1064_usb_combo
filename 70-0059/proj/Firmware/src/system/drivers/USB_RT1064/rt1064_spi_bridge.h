// rt1064_spi_bridge.h

#ifndef SRC_SYSTEM_DRIVERS_USB_HOST_RT1064_SPI_BRIDGE_H_
#define SRC_SYSTEM_DRIVERS_USB_HOST_RT1064_SPI_BRIDGE_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SPI_BRIDGE_FRAME_SIZE (16U)
#define SPI_BRIDGE_DATA_BYTES (16U)
#define SPI_BRIDGE_CRC_BYTES (0U)

extern volatile bool last_rx_good;
extern volatile uint32_t good_count;
extern volatile uint32_t bad_count;

extern volatile uint8_t last_rx[SPI_BRIDGE_FRAME_SIZE];
extern volatile uint8_t last_tx[SPI_BRIDGE_FRAME_SIZE];

extern volatile uint8_t spi_active_idx;
extern volatile uint8_t spi_last_completed_idx;
extern volatile uint32_t spi_transfer_seq;

/**
 * @brief Initialize the RT1064 SPI bridge task when the RT1064 USB host is present.
 */
void rt1064_spi_bridge_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_USB_HOST_RT1064_SPI_BRIDGE_H_ */
