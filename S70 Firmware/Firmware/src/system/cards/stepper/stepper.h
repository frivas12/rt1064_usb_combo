// stepper.h

#ifndef SRC_SYSTEM_CARDS_STEPPER_STEPPER_H_
#define SRC_SYSTEM_CARDS_STEPPER_STEPPER_H_

#include "compiler.h"
#include "user_spi.h"
#include "slots.h"
#include "encoder.h"
#include <arm_math.h>
#include "stepper_log.h"

#ifdef __cplusplus

#include "stepper_saves.h"

extern "C"
{
#endif

/****************************************************************************
 * Defines
 ****************************************************************************/
#define ST_STEPPER_SPI_MODE		_SPI_MODE_3
#define SPI_ST_STEPPER_NO_READ 	ST_STEPPER_SPI_MODE, SPI_NO_READ, CS_TOGGLE, SLOT_CS(slot)
#define SPI_ST_STEPPER_READ 	ST_STEPPER_SPI_MODE, SPI_READ, CS_TOGGLE, SLOT_CS(slot)

#define LOW_CURRENT_DRIVE 		0
#define HIGH_CURRENT_DRIVE 		1

#define HOMING_TIME	200	 /* use to search for index in ms => task rate 10ms so each count is 10ms */

#define STEPPER_SAVE_POSITION_IDLE_TIME	(10 * 100)  /* the task rate is 10ms so this gets incremented
													every 10 ms => 2 sec would be 2 * 100 = 200*/

#define STEPPER_NUM_OF_STORE_POS		10


//Status bits location
#define	STATUS_CW_LIMIT			1<<0
#define	STATUS_CCW_LIMIT		1<<1
#define	STATUS_CW_SOFT_LIMIT	1<<2
#define	STATUS_CCW_SOFT_LIMIT	1<<3
#define STATUS_CW_MOVE			1<<4
#define STATUS_CCW_MOVE			1<<5
#define STATUS_CW_JOG			1<<6
#define STATUS_CCW_JOG			1<<7
#define STATUS_MOTOR_CONN		1<<8
#define STATUS_HOMING			1<<9
#define STATUS_HOMED			1<<10
#define STATUS_INTERLOCK		1<<11 ///> \bug [COMPATABILITY] Should be 1 << 12; should act as the channel enable.
#define STATUS_ENC_WARN			1<<12
#define STATUS_ENC_ERROR		1<<13
#define STATUS_ENC_HI			1<<14
#define STATUS_ENC_LOW			1<<15
#define STATUS_ENC_READY		1<<16
#define STATUS_CHAN_EN			0x80000000
#define STATUS_SHUTTLE_ACTIVE	0x18000 // TODO change this was for the mesoscope

// Commands for ST L6470 stepper driver
//#define SET_PARAM_STEPPER			0x00	// SetParam(PARAM,VALUE),	Writes VALUE in PARAM register
//#define GET_PARAM_STEPPER			0x00	// GetParam(PARAM,VALUE),	Returns the stored value in PARAM register
#define RUN_STEPPER					0x50	// Run(DIR,SPD),			Sets the target speed and the motor direction
#define STEP_CLOCK_STEPPER			0x58	// StepClock(DIR),			Puts the device into Step-clock mode and imposes DIR direction
#define MOVE_STEPPER				0x40	// Move(DIR,N_STEP),		Makes N_STEP (micro)steps in DIR direction (Not performable when motor is running)
#define GOTO_STEPPER				0x60	// GoTo(ABS_POS),			Brings motor into ABS_POS position (minimum path)
#define GOTO_DIR_STEPPER			0x68	// GoTo_DIR(DIR,ABS_POS),	Brings motor into ABS_POS position forcing DIR direction
#define GO_UNTIL_STEPPER			0x82	// GoUntil(ACT,DIR,SPD),	Performs a motion in DIR direction with speed SPD until SW is closed,
//							the ACT action is executed then a SoftStop takes place.
//#define RELESE_SW_STEPPER			0x??	// ReleseSW(ACT, DIR),		Performs a motion in DIR direction at minimum
//							speed until the SW is released (open), the ACT
//							action is executed then a HardStop takes place.
#define GO_HOME_STEPPER				0x70	// GoHome,					Brings the motor into HOME position
#define GO_MARK_STEPPER				0x78	// GoMark,					Brings the motor into MARK position
#define RESET_POS_STEPPER			0xD8	// ResetPos,				Resets the ABS_POS register (set HOME position)
#define RESET_DEVICE_STEPPER		0xC0	// ResetDevice,				Device is reset to power-up conditions.
#define SOFT_STOP_STEPPER			0xB0	// SoftStop,				Stops motor with a deceleration phase
#define HARD_STOP_STEPPER			0xB8	// HardStop,				Stops motor immediately
#define SOFT_HiZ_STEPPER			0xA0	// SoftHiZ,					Puts the bridges into high impedance status
//							after a deceleration phase
#define HARD_HiZ_STEPPER			0xA8	// HardHiZ,					Puts the bridges into high impedance status immediately
#define GET_STATUS_STEPPER			0xD0	// GetStatus,				Returns the STATUS register value

// Stepper Status bits
#define HiZ					1<<0
#define BUSY				1<<1
#define SW_F				1<<2
#define SW_EVN				1<<3
#define DIR					1<<4
#define MOT_STATUS			3<<5
#define NOTPERF_CMD			1<<7
#define WRONG_CMD			1<<8
#define UVLO				1<<9
#define TH_WRN				1<<10
#define TH_SD				1<<11
#define OCD					1<<12
#define STEP_LOSS_A			1<<13	// low when stall detected
#define STEP_LOSS_B			1<<14
#define SCK_MOD				1<<15

// Motor Param Defines
#define ABS_POS         0x01        //length: 22      reset value: 0
#define EL_POS          0x02        //length: 9       reset value: 0
#define MARK            0x03        //length: 22      reset value: 0
#define SPEED           0x04        //length: 20      reset value: 0
#define ACC_L           0x05        //length: 12      reset value: 08A
#define DEC             0x06        //length: 12      reset value: 08A
#define MAX_SPEED       0x07        //length: 10      reset value: 41
#define MIN_SPEED       0x08        //length: 13      reset value: 0
#define FS_SPD          0x15        //length: 10      reset value: 027
#define KVAL_HOLD       0x09        //length: 8       reset value: 29
#define KVAL_RUN        0x0A        //length: 8       reset value: 29
#define KVAL_ACC        0x0B        //length: 8       reset value: 29
#define KVAL_DEC        0x0C        //length: 8       reset value: 29
#define INT_SPD         0x0D        //length: 14      reset value: 0408
#define ST_SLP          0x0E        //length: 8       reset value: 19
#define FN_SLP_ACC      0x0F        //length: 8       reset value: 29
#define FN_SLP_DEC      0x10        //length: 8       reset value: 29
#define K_THERM         0x11        //length: 4       reset value: 0
#define ADC_OUT         0x12        //length: 5       reset value:
#define OCD_TH          0x13        //length: 4       reset value: 8
#define STALL_TH        0x14        //length: 7       reset value: 40
#define STEP_MODE       0x16        //length: 8       reset value: 7
#define ALARM_EN        0x17        //length: 8       reset value: FF
#define CONFIG_L6470    0x18        //length: 16      reset value: 2E88
#define STATUS_L6470    0x19        //length: 16      reset value:

// only L6480
#define GATECFG1        0x18        //length: 11       reset value: 0
#define GATECFG2        0x19        //length: 8       reset value: 0
#define CONFIG_L6480    0x1A        //length: 16      reset value: 2C88
#define STATUS_L6480    0x1B        //length: 16      reset value:


/**
 * The stepper provides 
 */
typedef enum {
    /// \brief Absolute Movements (MOVE_ABSOLUTE, GOTO_STORED_POS)
    STEPPER_SPEED_CHANNEL_ABSOLUTE    = 0,

    /// \brief Relative Movements (MOVE_RELATIVE)
    STEPPER_SPEED_CHANNEL_RELATIVE    = 1,

    /// \brief Jogging Movements (MOVE_JOG)
    STEPPER_SPEED_CHANNEL_JOG         = 2,

    /// \brief Velocity Movements (MOT_VELOCITY, MCM_VELOCITY)
    STEPPER_SPEED_CHANNEL_VELOCITY    = 3,

    /// \brief Homing Movments (MOT_HOME)
    STEPPER_SPEED_CHANNEL_HOMING      = 4,

    /// \brief Joystick Speed (set on joystick input)
    STEPPER_SPEED_CHANNEL_JOYSTICK    = 5,

    /// \brief Synchronized Motion (currently unused)
    STEPPER_SPEED_CHANNEL_SYNC_MOTION = 6,

    /// \brief The maximum/hardware speed (L64-chip's MAX_SPEED register).
    STEPPER_SPEED_CHANNEL_UNBOUND     = 0xFF,
} stepper_speed_channel_e;

/****************************************************************************
 * Public Data
 ****************************************************************************/
typedef struct  __attribute__((packed))
{
	// Variables for PID algorithm
	float iterm; //!< Accumulator for integral term
	float lastin; //!< Last input value for differential term

	//variable to store relative velocity data from sm controller
	float max_velocity;
	int32_t error;
	uint8_t kickout_count;
} Stepper_Pid;


typedef struct  __attribute__((packed))
{
	int32_t cmnd_pos; 	/* (counts)	- used to check if move complete and got last mile*/
	int32_t goto_pos; 	/* (steps)	- used to send the goto to L6470 or L6480*/
	int32_t step_pos; 	/* (steps)	- stepper steps from driver.  22-bit signed value converted to a 32-bit signed value.*/
						// ! Do NOT set to a value outside of the 22-bit range.
	int32_t step_pos_32bit; 	/* (steps)	- stepper steps after expanding bits.  32-bit signed value.*/
	int16_t el_pos;		/* The EL_POS register contains the current electrical position of the motor.*/
	uint16_t homing_counter;
} Stepper_Counters;

/**
 * Stores the equivalent MAX_SPEED parameter for each
 * movement channel.
 */
typedef struct __attribute__((packed)) {
    uint32_t absolute;
    uint32_t relative;
    // uint32_t jog; is saved elsewhere
    uint32_t velocity;
    // uint32_t homing; is saved elsewhere
    uint32_t joystick;
    uint32_t sync_motion;
} Stepper_SpeedControl;

typedef struct  __attribute__((packed))
{
	bool which_stepper_drive;	// high or low current L6470 low L6480 high
	uint32_t status_bits;
	bool selected_stage;
	bool busy;
	uint8_t mode;
	uint8_t prev_flags;
	bool cmd_dir;              // The desired direction for the stepper's next movement command.
	bool cur_dir;              // The direction the stepper's last movement command (after possible reversal).
	uint32_t cmd_vel;          // The desired speed for the stepper's next run command.
                               // High-level code should preferr the speed channel system.
	uint32_t cur_vel;          // The speed of the stepper's last run command (after clamping).
	uint8_t goto_mode;
	uint8_t jog_mode;
	uint8_t homing_mode;
	uint8_t homing_control; /* used to spin the epi turret once to calibrate, then
							it is used to count the next position to calibrate
							*/

    stepper_speed_channel_e current_speed_channel;
    Stepper_SpeedControl speed_control;
} Stepper_Control;


/************************************************************************************
 * Public Function Prototypes
 ************************************************************************************/
void set_parameter(uint8_t slot, uint8_t parameter, uint8_t length,
		uint32_t val);
void get_parameter(uint8_t slot, uint8_t parameter, uint8_t length,
		uint8_t *spi_tx_data);
void run_stepper(uint8_t slot, uint8_t direction, uint32_t speed);
void step_clock_stepper(uint8_t slot, bool direction);
void move_stepper(uint8_t slot, bool direction, uint32_t n_step);
void goto_stepper(uint8_t slot, uint32_t abs_pos);
void goto_dir_stepper(uint8_t slot, bool direction, uint32_t abs_pos);
void goto_until_stepper(uint8_t slot, bool act, bool direction,
		uint32_t abs_pos);
void goto_home_stepper(uint8_t slot);
void goto_mark_stepper(uint8_t slot);
void reset_pos_stepper(uint8_t slot);
void reset_device_stepper(uint8_t slot);
void soft_stop_stepper(uint8_t slot);
void hard_stop_stepper(uint8_t slot);
void soft_hiz_stepper(uint8_t slot);
void hard_hiz_stepper(uint8_t slot);
uint16_t get_status_stepper(uint8_t slot);

void stepper_init(uint8_t slot, uint8_t type);

#ifdef __cplusplus
}
/************************************************************************************
 * C++ Only Data Types
 ************************************************************************************/

struct Stepper_info
{
	const slot_nums slot;
	bool channel_enable;
	uint32_t stage_moved_elapse_count;
	bool stage_position_saved;
	uint8_t current_stored_position;	// pos 1 = 0, pos 2 = 1, ... pos 10 = 9, not at any pos = 0xff
	Stepper_Counters counter;
	Encoder enc;
	Limits limits;
	Stepper_Control ctrl;
	Stepper_Pid pid;

	StepperSave save;

	uint16_t  timer;	/*Used for homing but can be used for other things as long as they are
						not used at the same time*/
	bool collision;
	bool soft_reboot;

	Stepper_info(const slot_nums slot);

	uint16_t stepper_status;

	// Set:  The stepper chip's status was read.
	// Reset:  The stepper chip's status has been exported (stepper log).  The stepper chip should be read again.
#if ENABLE_STEPPER_LOG_STATUS_FLAGS
	bool stepper_status_read;
#endif
};

/************************************************************************************
 * C++ Only Public Function Prototypes
 ************************************************************************************/
extern "C++" void get_el_pos_stepper(Stepper_info *info);
extern "C++" void sync_stepper_steps_to_encoder_counts(Stepper_info *info);
extern "C++" bool stepper_busy(Stepper_info *info);
extern "C++" bool check_run_limits(Stepper_info *info);
extern "C++" bool check_goto_limits(Stepper_info *info);
extern "C++" void update_status_bits(Stepper_info *info);
extern "C++" uint32_t stepper_get_speed_channel_value(const Stepper_info *info, stepper_speed_channel_e channel);
extern "C++" void stepper_set_speed_channel_value(Stepper_info *info, stepper_speed_channel_e channel, uint32_t value);
#endif

#endif /* SRC_SYSTEM_CARDS_STEPPER_STEPPER_H_ */
