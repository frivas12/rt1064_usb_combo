// ftdi.h

#ifndef SRC_DRIVERS_FTDI_H_
#define SRC_DRIVERS_FTDI_H_

#include "usb_slave.h"
#include <asf.h>

/****************************************************************************
 * Defines
 ****************************************************************************/
// #define FTDI_RX_BUFFER_SIZE USB_SLAVE_BUFFER_SIZE
// TODO (sbenish):  Optimize this size down later.
#define FTDI_RX_BUFFER_SIZE   300
#define FTDI_TX_BUFFER_SIZE   32
#define FTDI_TX_BUFFER_COUNT  6

#define FTDI_TIMEOUT 3000 // 150
#define FTDI_BUADRATE 115200 // 921600 256000 115200 460800
/*At 921600 bit takes 1.1us (so 1.1us x (1+8+1) = 11us
 * per byte */
/** XDMA channel used. */
#define FTDI_XDMA_CH 0

// static xdmac_channel_config_t xdmac_channel_cfg;
#define MICROBLOCK_LEN 16
#define BLOCK_LEN 1
#define UART1_DMAC_ID_TX 22

/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
iram_size_t ftdi_write(const void * type_erased_buf, iram_size_t nbytes);
void ftdi_init(void);
#endif /* SRC_DRIVERS_FTDI_H_ */
