// pins.h

#ifndef SRC_CONFIG_PINS_H_
#define SRC_CONFIG_PINS_H_

#include "component/pio.h"
#include "pio.h"
#include "ioport.h"

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
// (sbenish)  Defines like this are evil.  Changing.
// #define OFF		0
// #define ON		1
#define PIN_STATE_OFF   0
#define PIN_STATE_ON    1

/****************************************************************************
 * Pin assignments
 ****************************************************************************/
/*Extra pins uC, can be use to probe*/
#define PIN_M1		  			PIOA, 28

/**
 * Controls the high voltage to the system
 */
#define PIN_POWER_EN  			PIOD, 13

/**
 *  Lattice CPLD Program pins
 **/
#define PIN_CPLD_PROGRAMN		PIOD, 3
#define PIN_CPLD_RESET			PIOA, 23

/**
 * Analog pins
 */
#define PIN_TEMP_SENSOR			PIOB, 0
#define PIN_VIN_MONITOR			PIOB, 1

/**
 * SPI 0 pins
 */
#define PIN_MISO  				PIOD, 20
#define PIN_MOSI  				PIOD, 21
#define PIN_PCK  				PIOD, 22

/*
 *  Chip selects for SPI 0
 *  The SAMS70 only has 4 chip select pins for control and we need 5.  Each slot has up to 2 cs arranged
 *  as cs 1 and 2 are for slot 1, cs 3 and 4 are for slot 2 and so on.  A table is available in user_spi.c
  */
#define PIN_CS_0  				PIOB, 2		/* NPCS0 */
#define PIN_CS_1  				PIOA, 31	/*  NPCS1 */
#define PIN_CS_2  				PIOD, 12	/*  NPCS2 */
#define PIN_CS_3  				PIOD, 27	/*  NPCS3 */
#define PIN_CS_4  				PIOA, 8
#define PIN_C_CSSPIN			PIOA, 15	/*  Chip select for CPLD to communicate with uC*/
#define PIN_CPLD_CS				PIOA, 30	/*  Chip select for programming the CPLD */
#define PIN_CS_READY  			PIOA, 7

/*
 *  soft I2C
 */
#define PIN_I2C_SDA  			PIOA, 1
#define PIN_I2C_SCL  			PIOB, 4


/*
 * UART 0 for CPLD
 */
#define PIN_RX0  				PIOA, 9
#define PIN_TX0  				PIOA, 10

/*
 * UART 1 for FTDI
 */
#define PIN_UART_TX				PIOA, 4
#define PIN_UART_RX				PIOA, 5
#define PIN_FTDI_RTS			PIOD, 14
#define PIN_FTDI_CTS			PIOA, 27
#define PIN_FTDI_CBUS0			PIOD, 17

/*
 * Pins MAX3421 used for host USB.  MAX_SS  chip select tied to the CPLD CS controller.
 */
//
#define PIN_CLK_OUT 			PIOB, 12	// 12MHz clock from the processor
#define PIN_MAX_INT 			PIOA, 2
#define RT1064_INT_PIN			PIOA, 2		// Used on MCM_41_0117_RT1064 and newer
#define PIN_MAX_GPX_INT 		PIOD, 16	// not used
#define PIN_MAX_RES 			PIOA, 12	// Reset active low
#define PIN_MAX_PWR_EN 			PIOD, 9
#define PIN_MAX_PWR_FAULT		PIOD, 11

/*
 * Slot interrupt pins
 */
#define PIN_INT_1				PIOD, 10
#define PIN_INT_2				PIOD, 1
#define PIN_INT_3				PIOA, 11
#define PIN_INT_4				PIOA, 26
#define PIN_INT_5				PIOD, 28
#define PIN_INT_6				PIOA, 24
#define PIN_INT_7				PIOD, 18

/*
 * Direct IO pins from slots to processor ADC but can be used for IO
 */
#define PIN_ADC_1	  			PIOA, 19
#define PIN_ADC_2  				PIOA, 20
#define PIN_ADC_3  				PIOA, 17
#define PIN_ADC_4  				PIOA, 18
#define PIN_ADC_5  				PIOB, 3
#define PIN_ADC_6  				PIOA, 21
#define PIN_ADC_7  				PIOD, 30

/*
 * Direct IO pins from slots to processor
 */
#define PIN_SMIO_1  			PIOD, 26
#define PIN_SMIO_2  			PIOD, 25
#define PIN_SMIO_3  			PIOD, 2
#define PIN_SMIO_4  			PIOD, 24
#define PIN_SMIO_5  			PIOA, 13
#define PIN_SMIO_6  			PIOA, 14
#define PIN_SMIO_7  			PIOA, 25

/*
 * Clock to CPLD 12MHz
 * */
#define PIN_CPLD_CLK			PIOD, 31

/*
 * Extra pins from uC to CPLD
 */
#define PIN_CM_1  				PIOA, 16
#define PIN_CM_2  				PIOA, 22
#define PIN_CM_5  				PIOA, 0

/*
 * Extra pins that are tied to the uC and on a connector on some boards
 */
#define PIN_M1  				PIOA, 28
#define PIN_M2  				PIOD, 15
#define PIN_M3  				PIOD, 0
#define PIN_M4  				PIOD, 19

/*
 * Trace pins
 */
#define PIN_TRACED0  			PIOD, 4
#define PIN_TRACED1  			PIOD, 5
#define PIN_TRACED2  			PIOD, 6
#define PIN_TRACED3  			PIOD, 7
#define PIN_TRACECLK  			PIOD, 8

/****************************************************************************
 * Pin Functions
 ****************************************************************************/

__STATIC_INLINE void set_pin_as_output(Pio * port, uint32_t pin_index, bool level)
{
  const uint32_t MASK = pio_get_pin_group_mask(pin_index);
	(port->PIO_OER = MASK);

	/* Enable pin*/
	port->PIO_PER = MASK;

	/*Set level*/
	level == 1 ? (port->PIO_SODR = MASK) : (port->PIO_CODR = MASK);
}

__STATIC_INLINE void set_pin_as_input(Pio * port, uint32_t pin_index)
{
  const uint32_t MASK = pio_get_pin_group_mask(pin_index);
	port->PIO_ODR = MASK;
	port->PIO_OWER = MASK;

	/* Enable pin*/
	port->PIO_PER = MASK;

	/*Disable internal pullup*/
	port->PIO_PUDR = MASK;

	/*Disable internal pulldown*/
	port->PIO_PPDDR = MASK;
}

/**
 * This function is much quicker than the one in the asf.  The one in the asf take .2us this takes
 * .08us
 */
__STATIC_INLINE void pin_state( Pio * port, uint32_t pin_index, bool level)
{
  const uint32_t MASK = pio_get_pin_group_mask(pin_index);
	level == 1 ? (port->PIO_SODR = MASK) : (((Pio *)port)->PIO_CODR = MASK);
}

/**
 * This function is even faster than the one above "pin_state" if you know what state you want your pin, it
 * takes .06us
 */
__STATIC_INLINE void pin_high( Pio * port, uint32_t pin_index)
{
  const uint32_t MASK = pio_get_pin_group_mask(pin_index);
	(port->PIO_SODR = MASK);
}

/**
 * This function is even faster than the one above "pin_state" if you know what state you want your pin, it
 * takes .06us
 */
__STATIC_INLINE void pin_low( Pio * port, uint32_t pin_index)
{
  const uint32_t MASK = pio_get_pin_group_mask(pin_index);
	(port->PIO_CODR = MASK);
}

/*
 * Function to read pin level using pin definitions
 */
__STATIC_INLINE bool get_pin_level( Pio * port, uint32_t pin_index)
{
  const uint32_t MASK = pio_get_pin_group_mask(pin_index);
	return port->PIO_PDSR & MASK;
}


#ifdef __cplusplus
}
#endif

#endif /* SRC_CONFIG_PINS_H_ */
