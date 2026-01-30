////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ********** Defines **********
`define CPLD_REV							7
`define CPLD_REV_MINOR						0

`define DISABLED_MODE						0
`define ONE_WIRE_MODE						1
`define ENABLED_MODE						2
`define DIGITAL_OUTPUT						3

// stepper modes
`define STEP_MODE_ENC_BISS					4
`define STEP_MODE_MAG						5

// ********** Addresses **********
`define SLOT_1_ADDRESS						0
`define SLOT_2_ADDRESS						1
`define SLOT_3_ADDRESS						2
`define SLOT_4_ADDRESS						3
`define SLOT_5_ADDRESS						4
`define SLOT_6_ADDRESS						5
`define SLOT_7_ADDRESS						6
`define SLOT_8_ADDRESS						7

`define REVISION_ADDRESS					8
`define LED_ADDRESS							9
`define UART_CONTROLLER_ADDRESS				10

// ********** Set commands (16 bit) **********

// Enable card 1 to 47
`define C_SET_ENABLE_STEPPER_CARD			1
`define C_SET_ENABLE_SERVO_CARD				2
`define C_SET_ENABLE_OTM_DAC_CARD			3
`define C_SET_ENABLE_RS232_CARD				4
`define C_SET_ENABLE_IO_CARD				5
`define C_SET_ENABLE_SHUTTER_CARD			6
`define C_SET_ENABLE_PEIZO_ELLIPTEC_CARD	7
`define C_SET_ENABLE_PEIZO_CARD				8
	
// PWM commands
`define C_SET_PWM_FREQ						48
`define C_SET_PWM_DUTY						49

// Quad encoder commands
`define C_SET_QUAD_COUNTS					50
`define C_SET_HOMING						51

// Servo card commands
`define C_SET_SERVO_CARD_PHASE				52

// OTM Dac card commands
`define C_SET_OTM_DAC_LASER_EN				53

// uart commands
`define C_SET_UART_SLOT						54

// 4 shutter card commands
`define C_SET_SUTTER_PWM_DUTY				55
`define C_SET_SUTTER_PHASE_DUTY				56
`define C_SET_SUTTER_PWM_PERIOD             60

// IO card commands
`define C_SET_IO_PWM_DUTY					57

// Stepper card commands
//`define C_SET_STEPPER_ENC_NSL_DI			58
`define C_SET_STEPPER_DIGITAL_OUTPUT		59

// out-of-order
//`define C_SET_SUTTER_PWM_PERIOD             60


// ********** Read commands (16 bit) **********
// top bit must be 1 to indicate read command
`define C_READ_CPLD_REV						16'h8001
`define C_READ_INTERUPTS					16'h8002
`define C_READ_QUAD_COUNTS					16'h8003
`define C_READ_EM_STOP_FLAG					16'h8004
`define C_READ_QUAD_BUFFER					16'h8005
`define C_READ_BISS_ENC						16'h8006
`define C_READ_SSI_MAG						16'h8007


/////////////////////////////////////////


// global
//`define DEV_TYPE_STEPPER					8'h01
//`define DEV_TYPE_SERVO						8'h02
//`define DEV_TYPE_OTM_DAC					8'h03
//`define DEV_TYPE_RS232						8'h04
//`define DEV_TYPE_GALVO_DRIVER				8'h05


//`define UART_CONTROLLER_ADDRESS				8'h09
//`define FAN_ADDRESS							8'h0a

//`define STEPPER_RST_PIN						8'h40


//`define CS_FLASH							29
//`define CS_MAX3421						30

//`define M_DIR_IN							1'b0
//`define M_DIR_OUT							1'b1	



/////////////////////
// new commands (ZS)
/////////////////////
// global


//`define C_SET_DIG_OUT						8'h03
//`define C_SET_SERVO_PHASE					8'h04
//`define C_SET_UART_CHANNEL					8'h05

