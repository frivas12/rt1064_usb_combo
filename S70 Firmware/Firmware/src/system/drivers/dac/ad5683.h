/**
 * @file ad56x4.h
 *
 * @brief Functions for ???
 *
 */

#ifndef SRC_SYSTEM_DRIVERS_DAC_AD56X4_H_
#define SRC_SYSTEM_DRIVERS_DAC_AD56X4_H_

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
/*SPI*/
#define PIEZO_DAC_SPI_MODE		_SPI_MODE_1
#define SPI_PIEZO_DAC_NO_READ 	PIEZO_DAC_SPI_MODE, SPI_NO_READ, CS_NO_TOGGLE, SLOT_CS(slot)
#define SPI_PIEZO_DAC_READ 		PIEZO_DAC_SPI_MODE, SPI_READ, CS_NO_TOGGLE, SLOT_CS(slot)

//  **************** AD5683RBRMZ-RL7 16bit DAC ****************
#define DAC_DO_NOTHING					0
#define DAC_WRITE_INPUT_REG				1
#define DAC_UPDATE_DAC_REG				2
#define DAC_WRITE_DAC_AND_INPUT_REG		3
#define DAC_WRITE_CONTROL_REG			4
#define DAC_READBACK_INPUT_REG			5

//Write Control Register Bits
#define AD5683_DCEN(x)     (((((x) & 0x1) << 0) << 10) & 0xFC00)
#define AD5683_CFG_GAIN(x) (((((x) & 0x1) << 1) << 10) & 0xFC00)
#define AD5683_REF_EN(x)   (((((x) & 0x1) << 2) << 10) & 0xFC00)
#define AD5683_OP_MOD(x)   (((((x) & 0x3) << 3) << 10) & 0xFC00)
#define AD5683_SW_RESET(x) (((((x) & 0x1) << 5) << 10) & 0xFC00)

// Variable Declarations
enum ad5683_state
{
	AD5683_DC_DISABLE, //  daisy-chain
	AD5683_DC_ENABLE
};

enum ad5683_voltage_ref
{
	AD5683_INT_REF_ON, AD5683_INT_REF_OFF
};

enum ad5683_amp_gain
{
	AD5683_AMP_GAIN_1, AD5683_AMP_GAIN_2
};

enum ad5683_power_mode
{
	AD5683_PWR_NORMAL, AD5683_PD_1K, AD5683_PD_100K, AD5683_PD_3STATE
};

enum ad5683_reset
{
	AD5683_RESET_DISABLE, AD5683_RESET_ENABLE
};

//  END **************** AD5683RBRMZ-RL7 16bit DAC ****************

// AD5683RBRMZ-RL7 16 bit dac
// C0 - C3 Command defines bits 23:20 out of 24 bits
/*COMMANDS
 Write Input Register
 The input register allows the preloading of a new value for the
 DAC register. The transfer from the input register to the DAC
 register can be triggered by hardware, by the LDAC pin, or by
 software using Command 2.
 If new data is loaded into the DAC register directly using
 Command 3, the DAC register automatically overwrites the
 input register.

 Update DAC Register
 This command transfers the contents of the input register to the
 DAC register and, consequently, the VOUT pin is updated.
 This operation is equivalent to a software LDAC.

 Write DAC Register
 The DAC register controls the output voltage in the DAC. This
 command updates the DAC register on completion of the write
 operation. The input register is refreshed automatically with the
 DAC register value.

 Write Control Register
 The write control register sets the power-down and gain
 functions. It also enables/disables the internal reference and
 perform a software reset. See Table 10 for the write control
 register functionality.
 Table 10. Write Control Register Bits
 DB19 DB18 DB17 DB16 DB15 DB14
 Reset PD1 PD0 REF Gain DCEN
 */
/****************************************************************************
 * Public Data
 ****************************************************************************/

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void dac_init(slot_nums slot, uint16_t data);
void dac_write(slot_nums slot, uint8_t cmd, uint16_t data);


#ifdef __cplusplus
}
#endif

#endif /* SRC_SYSTEM_DRIVERS_DAC_AD56X4_H_ */
