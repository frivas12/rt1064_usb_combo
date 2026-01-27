/**
 * @file mcp23s09.c
 *
 * @brief Functions for MCP23S09.  These function calls require to take the SPI
 * seamaphore inside the calling function
 *
 * The GPIO module is a general purpose 8-bit wide bidirectional port.  The outputs
 * are open-drain.  The GPIO module contains the data ports (GPIOn), internal pull-up
 * resistors and the output latches (OLATn).  The pull-up resistors are individually
 * configured and can be enabled when the pin is configured as an input or output.
 * Reading the GPIOn register reads the value on the port. Reading the OLATn register
 * only reads the latches, not the actual value on the port.
 *
 * Writing to the GPIOn register actually causes a write to the latches (OLATn).
 * Writing to the OLATn register forces the associated output drivers to drive to the
 * level in OLATn. Pins configured as inputs turn off the associated output driver and
 * put it in high impedance.
 *
 */

#include <asf.h>
#include "mcp23s09.h"
#include <cpld.h>

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
static void write_register(uint8_t slot, uint8_t reg, uint8_t data);
static uint8_t read_register(uint8_t slot, uint8_t reg);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
// MARK:  SPI Mutex Required
static void write_register(uint8_t slot, uint8_t reg, uint8_t data)
{
	uint8_t spi_tx_data[3];

	spi_tx_data[0] = 0x40;		/* control byte always same*/
	spi_tx_data[1] = reg;		/* register address*/
	spi_tx_data[2] = data;		/* data*/

	spi_transfer(SPI_MCP23S09_NO_READ, spi_tx_data, 3);
}

// MARK:  SPI Mutex Required
static uint8_t read_register(uint8_t slot, uint8_t reg)
{
	uint8_t spi_tx_rx_data[3];

	spi_tx_rx_data[0] = 0x41;		/* control byte always same*/
	spi_tx_rx_data[1] = reg;		/* register address*/
	spi_tx_rx_data[2] = 0x00;		/* data*/

	spi_transfer(SPI_MCP23S09_READ, spi_tx_rx_data, 3);
	return spi_tx_rx_data[2];
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/**
  * @brief This register controls the direction of the data I/O.
 * When a bit is set, the corresponding pin becomes an
 * input. When a bit is clear, the corresponding pin
 * becomes an output.
 *
 * @param slot
 * @param io_dir = 1 = pin as an input, 0 = pin as an output
 */
// MARK:  SPI Mutex Required
void write_mcp_io_dir(uint8_t slot, uint8_t io_dir)
{
	write_register(slot, IODIR, io_dir);
}

// MARK:  SPI Mutex Required
uint8_t read_mcp_io_dir(uint8_t slot)
{
	return read_register(slot, IODIR);
}

/**
 * @brief This register allows the user to configure the polarity on the corresponding
 *  GPIO port bits.  If a bit is set, the corresponding GPIO register bit will reflect
 *  the inverted value on the pin.
 *  1 = GPIO register bit will reflect the opposite logic state of the input pin
 *  0 = GPIO register bit will reflect the same logic state of the input pin
 * @param info
 */
// MARK:  SPI Mutex Required
void write_mcp_input_polarity(uint8_t slot, uint8_t polarity)
{
	write_register(slot, IPOL, polarity);
}

/**
 * @brief The GPINTEN register controls the
 * Interrupt-on-Change feature for each pin.
 * If a bit is set, the corresponding pin is enabled for
 * Interrupt-on-Change. The DEFVAL and INTCON
 * registers must also be configured if any pins are
 * enabled for Interrupt-on-Change.
 * @param slot
 * @param io_interrupt_on_change
 */
// MARK:  SPI Mutex Required
void write_mcp_interrupt_on_change(uint8_t slot, uint8_t io_interrupt_on_change)
{
	write_register(slot, GPINTEN, io_interrupt_on_change);
}

/**
 * @brief The default comparison value is configured in the
 * DEFVAL register. If enabled (via GPINTEN and
 * INTCON) to compare against the DEFVAL register, an
 * opposite value on the associated pin will cause an
 * Interrupt to occur.
 * @param slot
 * @param io_compare
 */
// MARK:  SPI Mutex Required
void write_mcp_compare_reg(uint8_t slot, uint8_t io_compare)
{
	write_register(slot, DEFVAL, io_compare);
}

/**
 * @brief The INTCON register controls how the associated pin
* value is compared for the Interrupt-on-Change feature.
* If a bit is set, the corresponding I/O pin is compared
* against the associated bit in the DEFVAL register. If a
* bit value is clear, the corresponding I/O pin is compared
* against the previous value.
 * @param slot
 * @param io_int_comp_reg,
 * 1 = Pin value is compared against the associated bit in the DEFVAL register
 * 0 = Pin value is compared against the previous pin value
 */
// MARK:  SPI Mutex Required
void write_mcp_interrupt_control_reg(uint8_t slot, uint8_t io_int_comp_reg)
{
	write_register(slot, INTCON, io_int_comp_reg);
}

/**
 * @brief The Sequential Operation (SEQOP) bit controls the
 * incrementing function of the address pointer. If the
 * address pointer is disabled, the address pointer does
 * not automatically increment after each byte is clocked
 * during a serial transfer. This feature is useful when it is
 * desired to continuously poll (read) or modify (write) a
 * register
 *
 * The Open-Drain (ODR) control bit enables/disables the
 * INT pin for open-drain configuration.
 *
 * The Interrupt Polarity (INTPOL) bit sets the polarity of
 * the INT pin. This bit is functional only when the ODR bit
 * is cleared, configuring the INT pin as active push-pull.
 * The Interrupt Clearing Control (INTCC) bit configures
 * how Interrupts are cleared. When set (INTCC = 1), the
 * Interrupt is cleared when the INTCAP register is read.
 * When cleared (INTCC = 0), the Interrupt is cleared
 * when the GPIO register is read.
 *
 * The Interrupt can only be cleared when the Interrupt
 * condition is inactive.
 * @param slot
 * @param io_config_reg
 * bit 5 SEQOP: Sequential Operation mode bit
 * 1 = Sequential operation disabled, address pointer does not increment
 * 0 = Sequential operation enabled, address pointer increments
 *
 * bit 2 ODR: Configures the INT pin as an open-drain output.
 * 1 = Open-drain output (overrides the INTPOL bit)
 * 0 = Active driver output (INTPOL bit sets the polarity)
 *
 * bit 1 INTPOL: Sets the polarity of the INT output pin.
 * 1 = Active-High
 * 0 = Active-Low
 *
 * bit 0 INTCC: Interrupt Clearing Control
 * 1 = Reading INTCAP register clears the Interrupt
 * 0 = Reading GPIO register clears the Interrupt
 */
// MARK:  SPI Mutex Required
void write_mcp_config_reg(uint8_t slot, uint8_t io_config_reg)
{
	write_register(slot, IOCON, io_config_reg);
}

/**
 * @brief The GPPU register controls the pull-up resistors for the
* port pins. If a bit is set, the corresponding port pin is
* internally pulled up with an internal resistor
 * @param slot
 * @param io_pullup, 1 = Pull-Up enabled, 0 = Pull-Up disabled
 */
// MARK:  SPI Mutex Required
void write_mcp_pullup_reg(uint8_t slot, uint8_t io_pullup)
{
	write_register(slot, GPPU, io_pullup);
}

/**
 * @brief The INTF register reflects the Interrupt condition on the
 * port pins of any pin that is enabled for interrupts via the
 * GPINTEN register. A set bit indicates that the
 * associated pin caused the Interrupt.
 * This register is read-only. Writes to this register will be
 * ignored.
 * @param slot
 * @param io_interrupt_flags
 * @return Reflects the interrupt condition on the port. Will reflect the
 * change only if interrupts are enabled (GPINTEN) <7:0>.
 * 1 = Pin caused Interrupt
 * 0 = Interrupt not pending
 */
// MARK:  SPI Mutex Required
uint8_t read_mcp_interrupt_flag_reg(uint8_t slot)
{
	return read_register(slot, INTF);
}

/**
 * @brief The INTCAP register captures the GPIO port value at
 * the time the Interrupt occurred. The register is
 * read-only� and is updated only when an Interrupt
 * occurs. The register will remain unchanged until the
 * Interrupt is cleared via a read of INTCAP or GPIO.
 * @param slot
 * @return  Reflects the logic level on the port pins at the time of
 * Interrupt due to pin change <7:0>
 * 1 = Logic-High
 * 0 = Logic-Low
 */
// MARK:  SPI Mutex Required
uint8_t read_mcp_interrupt_capture_reg(uint8_t slot)
{
	return read_register(slot, INTCAP);
}

/**
 * @brief The GPIO register reflects the value on the port.
 *  Writing to this register modifies the Output Latch (OLAT) register.
 * @param slot
 * @param io_pin_state, 1 = Logic-High, 0 = Logic-Low
 */
// MARK:  SPI Mutex Required
void write_mcp_port_reg(uint8_t slot, uint8_t io_pin_state)
{
	write_register(slot, GPIO, io_pin_state);

}

/**
 * @brief The GPIO register reflects the value on the port.
 * Reading from this register reads the port
 * @param slot
 * @param io_pin_state, 1 = Logic-High, 0 = Logic-Low
 */
// MARK:  SPI Mutex Required
uint8_t read_mcp_port_reg(uint8_t slot)
{
	return read_register(slot, GPIO);
}

/**
 * @brief The OLAT register provides access to the output
 * latches. A write to this register modifies the output latches
 * that modifies the pins configured as outputs.
 *
 * @param slot
 * @param io_latch, 1 = Logic-High, 0 = Logic-Low
 */
// MARK:  SPI Mutex Required
void write_mcp_output_latch_reg(uint8_t slot, uint8_t io_pin_state)
{
	write_register(slot, OLAT, io_pin_state);
}

/**
 * @brief The OLAT register provides access to the output
 * latches. A read from this register results in a read of the
 * OLAT and not the port itself.
 *
 * @param slot
 * @param io_latch, 1 = Logic-High, 0 = Logic-Low
 */
// MARK:  SPI Mutex Required
uint8_t read_mcp_output_latch_reg(uint8_t slot)
{
	return read_register(slot, OLAT);
}


