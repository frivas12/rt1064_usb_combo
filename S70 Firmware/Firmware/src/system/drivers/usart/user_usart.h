/**
 * @file user_usart.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_USART_USER_USART_H_
#define SRC_SYSTEM_DRIVERS_USART_USER_USART_H_

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
#define UART0_BUFFER_SIZE			100
#define DISABLE_UART0_INTERRUPT		0
#define ENABLE_UART0_INTERRUPT		1

/****************************************************************************
 * Public Data
 ****************************************************************************/
/* Pointer to structure for holding the USB data from rx*/
extern fifo_t *uart0_fifo_rx;

typedef struct
{
	bool enable_int;
	uint32_t baudrate;
	uint8_t address;
	TaskHandle_t task_using;
} Uart0;
extern Uart0 uart0;

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void int2Hex(int32_t intnumber, uint8_t *txt);
uint32_t hex2int(uint8_t *hex);
void uart0_init(void);
void uart0_set_channel( uint8_t slot, uint32_t baudrate, uint8_t address);

void uart0_setup_read(uint8_t bytes_to_read);
bool uart0_write(TickType_t delay, const uint8_t * p_data, uint8_t bytes_to_write);
bool uart0_read(TickType_t delay, uint8_t * p_data);
void uart0_wait_until_tx_done(void);

extern SemaphoreHandle_t xUART0_Semaphore;

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_USART_USER_USART_H_ */
