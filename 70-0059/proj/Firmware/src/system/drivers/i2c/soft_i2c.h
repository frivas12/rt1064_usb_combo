// soft_i2c.h

#ifndef SRC_INCLUDE_SOFT_I2C_H_
#define SRC_INCLUDE_SOFT_I2C_H_

#include <asf.h>
#include <pins.h>
#include "delay.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
/*
 * For high signal set as input pullup will pull high on open drain
 *  for low set as output low
 */
__STATIC_INLINE void i2c_high(Pio * port, uint32_t pin)
{
	set_pin_as_input(port, pin); // this floats the pin as open drain
	delay_us(4);
}

__STATIC_INLINE void i2c_low(Pio * port, uint32_t pin)
{
	set_pin_as_output(port, pin, 0);
	delay_us(2);
}

/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void soft_i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
uint8_t i2c_read(uint8_t ack);
bool i2c_write(uint8_t data);
void set_channel(uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif /* SRC_INCLUDE_SOFT_I2C_H_ */
