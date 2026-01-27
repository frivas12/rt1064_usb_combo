/*
 * cpld.h
 *
 *  Created on: Jul 22, 2015
 *      Author: frivas
 */

#ifndef SRC_USR_CPLD_CPLD_H_
#define SRC_USR_CPLD_CPLD_H_

#include "usb_slave.h"
#include "user_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
#define CPLD_SPI_MODE _SPI_MODE_0
#define SPI_CPLD_NO_READ CPLD_SPI_MODE, SPI_NO_READ, CS_NO_TOGGLE, CS_CPLD
#define SPI_CPLD_READ CPLD_SPI_MODE, SPI_READ, CS_NO_TOGGLE, CS_CPLD

// ********** Defines **********
#define DISABLED_MODE 0
#define ONE_WIRE_MODE 1
#define ENABLED_MODE 2
#define DIGITAL_OUTPUT 3

// stepper modes
#define STEP_MODE_ENC_BISS 4
#define STEP_MODE_MAG 5

#define C_HOMING_DISABLE 0
#define C_HOMING_ENABLE 1
#define C_HOMING_MULT_INDEX 2

// ********** Addresses **********
#define SLOT_1_ADDRESS 0
#define SLOT_2_ADDRESS 1
#define SLOT_3_ADDRESS 2
#define SLOT_4_ADDRESS 3
#define SLOT_5_ADDRESS 4
#define SLOT_6_ADDRESS 5
#define SLOT_7_ADDRESS 6
#define SLOT_8_ADDRESS 7 // this is the half slot

#define REVISION_ADDRESS 8
#define LED_ADDRESS 9
#define UART_CONTROLLER_ADDRESS 10

// ********** Set commands (16 bit) **********

// Enable card range 1 to 47
#define C_SET_ENABLE_STEPPER_CARD 1
#define C_SET_ENABLE_SERVO_CARD 2
#define C_SET_ENABLE_OTM_DAC_CARD 3
#define C_SET_ENABLE_RS232_CARD 4
#define C_SET_ENABLE_IO_CARD 5
#define C_SET_ENABLE_SHUTTER_CARD 6
#define C_SET_ENABLE_PEIZO_ELLIPTEC_CARD 7
#define C_SET_ENABLE_PEIZO_CARD 8

// PWM commands
#define C_SET_PWM_FREQ 48
#define C_SET_PWM_DUTY 49

// Quad encoder commands
#define C_SET_QUAD_COUNTS 50
#define C_SET_HOMING 51

// Servo card commands
#define C_SET_SERVO_CARD_PHASE 52

// OTM Dac card commands
#define C_SET_OTM_DAC_LASER_EN 53

// uart commands
#define C_SET_UART_SLOT 54

// 4 shutter card commands
#define C_SET_SUTTER_PWM_DUTY 55
#define C_SET_SUTTER_PHASE_DUTY 56

// IO card commands
#define C_SET_IO_PWM_DUTY 57

// Stepper card commands
// #define C_SET_STEPPER_ENC_NSL_DI			58
#define C_SET_STEPPER_DIGITAL_OUTPUT 59

// ********** Read commands (16 bit) **********
// top bit must be 1 to indicate read command
#define C_READ_CPLD_REV 0x8001
#define C_READ_INTERUPTS 0x8002
#define C_READ_QUAD_COUNTS 0x8003
#define C_READ_EM_STOP_FLAG 0x8004
#define C_READ_QUAD_BUFFER 0x8005
#define C_READ_BISS_ENC 0x8006
#define C_READ_SSI_MAG 0x8007

////////////////////////////

// #define C_SET_DIG_OUT						0x03
// // 8'h03 #define DEV_TYPE_SERVO
// 0x02	// 8'h02 #define DEV_TYPE_OTM_DAC
// 0x03	// 8'h03 #define DEV_TYPE_RS232
// 0x04	// 8'h04 #define C_SET_SLOT_TYPE_CONFIG
// 0x01	// 8'h01 #define C_SET_SERVO_PHASE
// 0x04	// 8'h04

/****************************************************************************
 * Public Data
 ****************************************************************************/
extern uint8_t cpld_tx_data[6];
extern uint8_t cpld_rx_data[6];
extern bool cpld_programmed;
extern uint8_t cpld_revision;
extern uint8_t cpld_revision_minor;

/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void set_reg(uint16_t command, uint8_t address, uint8_t mid_data,
             uint32_t data);
void read_reg(uint16_t command, uint8_t address, uint8_t *mid_data,
              uint32_t *data);
void cpld_reset(void);
bool cpld_read_em_stop_flag(void);
void cpld_init(void);
void cpld_write_read(USB_Slave_Message *slave_message);
void cpld_start_slot_cards(void);

#ifdef __cplusplus
}
#endif

#endif /* SRC_USR_CPLD_CPLD_H_ */
