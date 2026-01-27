// user_spi.h

#ifndef SRC_SYSTEM_DRIVERS_SPI_USER_SPI_H_
#define SRC_SYSTEM_DRIVERS_SPI_USER_SPI_H_

#include "compiler.h"
#include "FreeRTOS.h"
#include "semphr.h"

#include "spi.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
/* Delay before SPCK. */
#define SPI_DLYBS 3// 0x40

/* Delay between consecutive transfers. */
#define SPI_DLYBCT 3//0x10

/* SPI clock setting (Hz). */
#define SPI_SPEED		5000000
#define SPI_SLOW_SPEED	1000000

#define DUMMY_LOW		0x00
#define DUMMY_HIGH		0xFF

/* Chip select. */
#define CS_CPLD_PROGRAM			63
#define CS_CPLD					95
#define CS_ALL_HIGH				96
#define CS_FLASH				125
#define CS_MAX3421				126
#define CS_RT_UPDATE			124

typedef enum
{
	_SPI_MODE_0 = 0, _SPI_MODE_1, _SPI_MODE_2, _SPI_MODE_3
} spi_modes;

#define CS_NO_TOGGLE	0
#define CS_TOGGLE		1

#define SPI_NO_READ		0
#define SPI_READ		1

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern SemaphoreHandle_t xSPI_Semaphore;

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void user_spi_init(void);
spi_status_t spi_transfer(spi_modes mode, bool read, bool toggle, uint8_t cs, uint8_t *buf, uint32_t size);

spi_status_t spi_start_transfer(spi_modes mode, bool toggle, uint8_t cs);
spi_status_t spi_partial_write(uint8_t value);
spi_status_t spi_partial_transfer(uint8_t* inout);
spi_status_t spi_partial_write_array(const uint8_t *buf, uint32_t size);
spi_status_t spi_partial_transfer_array(uint8_t *buf, uint32_t size);
spi_status_t spi_end_transfer(void);
#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_SPI_USER_SPI_H_ */
