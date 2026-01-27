/**
 * @file mcp23s09.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_IO_MCP23S09_H_
#define SRC_SYSTEM_DRIVERS_IO_MCP23S09_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
#define GPIO_ALL_OUTPUTS	0x00
#define GPIO_ALL_INPUTS		0xFF

/*Registers*/
#define IODIR		 			0x00
#define IPOL		 			0x01
#define GPINTEN		 			0x02
#define DEFVAL		 			0x03
#define INTCON		 			0x04
#define IOCON		 			0x05
#define GPPU		 			0x06
#define INTF		 			0x07
#define INTCAP		 			0x08
#define GPIO		 			0x09
#define OLAT		 			0x0A

/*SPI settings*/
#define MCP23S09_SPI_MODE		_SPI_MODE_3
#define SPI_MCP23S09_NO_READ 	MCP23S09_SPI_MODE, SPI_NO_READ, CS_NO_TOGGLE, SLOT_CS(slot)
#define SPI_MCP23S09_READ 		MCP23S09_SPI_MODE, SPI_READ, CS_NO_TOGGLE, SLOT_CS(slot)

/*Register bits*/

/*Configuration register*/
#define INTCC	 				1 << 0
#define INTPOL	 				1 << 1
#define ODR		 				1 << 2
#define SEQOP		 			1 << 5

/****************************************************************************
 * Public Data
 ****************************************************************************/
typedef struct  __attribute__((packed))
{
	uint8_t io_dir;		/*Holds the direction for each pin. 1 = input, 0 = output */
	uint8_t io_polarity;
	uint8_t io_interrupt_on_change;
	uint8_t io_compare;
	uint8_t io_int_comp_reg;
	uint8_t io_config_reg;
	uint8_t io_pullup;
	uint8_t io_interrupt_flags;
	uint8_t interrupt_capture_reg;
	uint8_t io_pin_state;
	uint8_t io_latch;
} mcp23s09_info;

typedef struct
{
	uint8_t io_dir;		/*Holds the direction for each pin. 1 = input, 0 = output */
	uint8_t io_polarity;
	uint8_t io_interrupt_on_change;
	uint8_t io_compare;
	uint8_t io_int_comp_reg;
	uint8_t io_config_reg;
	uint8_t io_pullup;
	uint8_t io_interrupt_flags;
	uint8_t interrupt_capture_reg;
	uint8_t io_pin_state;
	uint8_t io_latch;
} mcp23s09_state;


/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void write_mcp_io_dir(uint8_t slot, uint8_t io_dir);
void write_mcp_input_polarity(uint8_t slot, uint8_t polarity);
void write_mcp_interrupt_on_change(uint8_t slot, uint8_t io_interrupt_on_change);
void write_mcp_compare_reg(uint8_t slot, uint8_t io_compare);
void write_mcp_interrupt_control_reg(uint8_t slot, uint8_t io_int_comp_reg);
void write_mcp_config_reg(uint8_t slot, uint8_t io_config_reg);
void write_mcp_pullup_reg(uint8_t slot, uint8_t io_pullup);
uint8_t read_mcp_interrupt_flag_reg(uint8_t slot);
uint8_t read_mcp_interrupt_capture_reg(uint8_t slot);
void write_mcp_port_reg(uint8_t slot, uint8_t io_pin_state);
uint8_t read_mcp_io_dir(uint8_t slot);
uint8_t read_mcp_port_reg(uint8_t slot);
void write_mcp_output_latch_reg(uint8_t slot, uint8_t io_pin_state);
uint8_t read_mcp_output_latch_reg(uint8_t slot);

#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_IO_MCP23S09_H_ */
