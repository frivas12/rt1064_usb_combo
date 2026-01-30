//stepper_LUT.h

/**************************************************************************//**
 * \file stepper_LUT.h
 * \author Sean Benish
 * \brief Holds all the data structures that a stepper will save onto the LUT.
 *****************************************************************************/

#ifndef SRC_SYSTEM_CARDS_STEPPER_STEPPER_SAVES_H_
#define SRC_SYSTEM_CARDS_STEPPER_STEPPER_SAVES_H_

#include "usr_limits.h"
#include "encoder.h"
#ifndef _DESKTOP_BUILD // Used in the LUT compiler to ignore this import.
#include "arm_math.h"
#else
typedef float float32_t;
#endif

#include "defs.h"

#ifdef __cplusplus
#include "eeprom_wrapped.tcc"

extern "C"
{
#endif

/*****************************************************************************
 * Defines
 *****************************************************************************/

#define STEPPER_SUBDEVICES              1
#define STEPPER_CONFIGS_IN_SAVE         8
#define STEPPER_CONFIGS_TOTAL           9

/*****************************************************************************
 * Data Types
 *****************************************************************************/

typedef struct  __attribute__((packed))
{
    uint32_t axis_serial_no;
    float32_t counts_per_unit;  ///> Keep this in here, as this is affected by gearboxes.
    uint32_t min_pos;
    uint32_t max_pos;
    uint16_t collision_threshold;
} Stepper_Config;

typedef struct  __attribute__((packed))
{
    /*These are for the drive only*/
    uint32_t acc; /*register 0x05, Acceleration*/
    uint32_t dec; /*register 0x06, Deceleration*/
    uint32_t max_speed; /*register 0x07, Maximum speed*/
    uint16_t min_speed; /*register 0x08, Minimum speed*/
    uint16_t fs_spd; /*register 0x15, Full-step speed*/
    uint8_t kval_hold; /*register 0x09, Holding KVal*/
    uint8_t kval_run; /*register 0x0A, Constant speed KVal*/
    uint8_t kval_acc; /*register 0x0B, Acceleration starting KVal*/
    uint8_t kval_dec; /*register 0x0C, Deceleration starting KVal*/
    uint16_t int_speed; /*register 0x0D, Intersect speed*/
    uint8_t stall_th; /*register 0x13, STALL threshold stall threshold 0 = 31.25mA max 127 = 4A*/
    uint8_t st_slp; /*register 0x0E, Start slope*/
    uint8_t fn_slp_acc; /*register 0x0F, Acceleration final slope*/
    uint8_t fn_slp_dec; /*register 0x10, Deceleration final slope*/
    uint8_t ocd_th; /*register 0x13, OCD threshold*/
    uint8_t step_mode; /*register 0x16, Step mode*/
    uint16_t config; /*register 0x18(L6470) 0x1A(L6480), IC configuration */
    uint16_t gatecfg1; /*register 0x18, Gate driver configuration, (L6480 ONLY) */
    uint8_t gatecfg2; /*register 0x19, Gate driver configuration, (L6480 ONLY) */ ///\bug This needs to be 16 bits.
    uint16_t approach_vel;
    uint16_t deadband;
    uint16_t backlash; // 43rd byte
    uint8_t kickout_time;/*  This is used so that once the pid get to the commanded position
                                                        and the kickout is active, the pid will keep trying to get to
                                                        the commanded position for this about of time.  In 10's of ms */
} Stepper_drive_params;

typedef struct  __attribute__((packed))
{
    uint8_t flags;          /* bit[0] 1 for has encoder
                                                bit[1] 1 for reverse encoder count,
                                                bit[2] 1 for has index,
                                                bit[3] 1 for reverse stepper count,
                                                bit[4] 1 to enable PID for goto and jog commands,
                                                bit[5] 1 to enable PID kickout (this stops the PID controller when
                                                the destination is reached)
                                                bit[6] 1 for rotational stages, 0 for linear stages.
                                                bit[7] 1 for prefer soft stop, 0 for prefer hard stop
                                                */
} Flags_save;

typedef struct  __attribute__((packed))
{
    uint8_t home_mode;
    uint8_t home_dir;
    uint16_t limit_switch;
    uint32_t home_velocity; /* 0-100 =>  max_speed * (100/home_velocity) */
    int32_t offset_distance; // in encoder counts
} Home_Params;

typedef struct  __attribute__((packed))
{
    int16_t jog_mode;
    int32_t step_size;
    int32_t min_vel;
    int32_t acc;
    int32_t max_vel;
    int16_t stop_mode;
} Jog_Params;

typedef struct  __attribute__((packed))
{
    // Tuning parameters
    uint32_t Kp; //!< Stores the gain for the Proportional term
    uint32_t Ki; //!< Stores the gain for the Integral term
    uint32_t Kd; //!< Stores the gain for the Derivative term
    uint32_t imax;
    uint16_t FilterControl; // not used
} Stepper_Pid_Save;

typedef struct  __attribute__((packed))
{
    int32_t enc_pos;        /* (counts)     - encoder counts*/
    uint16_t el_pos;        /* The EL_POS register contains the current electrical position of the motor.*/
    int32_t enc_zero;
    int32_t stored_pos[NUM_OF_STORED_POS];
    uint16_t stored_pos_deadband;
} Stepper_Store;

typedef struct __attribute__((packed))
{
    Stepper_Config config;
    Stepper_drive_params drive;
    Flags_save flags;
    Limits_save limits;
    Home_Params home;
    Jog_Params jog;
    Encoder_Save encoder;
    Stepper_Pid_Save pid;
} Stepper_Parameters;

typedef struct __attribute__((packed))
{
    config_signature_t stepper_config;
    config_signature_t stepper_drive_params;
    config_signature_t flags_save;
    config_signature_t limits_save;
    config_signature_t home_params;
    config_signature_t jog_params;
    config_signature_t enc_save;
    config_signature_t pid_save;
    config_signature_t stepper_store;
} Stepper_Save_Configs;

typedef union __attribute__((packed))
{
    config_signature_t array[STEPPER_CONFIGS_TOTAL];
    Stepper_Save_Configs cfg;
} Stepper_Save_Union;

#ifdef __cplusplus
}

/**
 * A class that provides an interface for implementing sub-field
 * saves required by the MGMSG_MOT_SET_EEPROMPARAMS command.
 * It also has information on if the parameterization structure
 * has previously been configured.
 */
class StepperSavedConfigs
{
private:
    // BEGIN    Compile-Time EEPROM Offsets
    constexpr static std::size_t CONFIG_OFFSET = sizeof(uint64_t) + sizeof(CRC8_t);
    constexpr static std::size_t DRIVE_OFFSET = CONFIG_OFFSET + sizeof(Stepper_Config);
    constexpr static std::size_t FLAGS_OFFSET = DRIVE_OFFSET + sizeof(Stepper_drive_params);
    constexpr static std::size_t LIMITS_OFFSET = FLAGS_OFFSET + sizeof(Flags_save);
    constexpr static std::size_t HOME_OFFSET = LIMITS_OFFSET + sizeof(Limits_save);
    constexpr static std::size_t JOG_OFFSET = HOME_OFFSET + sizeof(Home_Params);
    constexpr static std::size_t ENCODER_OFFSET = JOG_OFFSET + sizeof(Jog_Params);
    constexpr static std::size_t PID_OFFSET = ENCODER_OFFSET + sizeof(Encoder_Save);
    // END      Compile-Time EEPROM Offsets


    // Slim wrappers allow for the stepper saved configs to define the structure
    // cutting down on memory cost.

    EepromMapperSlim _config_mapper;
    EepromMapperSlim _drive_mapper;
    EepromMapperSlim _flags_mapper;
    EepromMapperSlim _home_mapper;
    EepromMapperSlim _jog_mapper;
    EepromMapperSlim _pid_mapper;
    EepromMapperSlim _encoder_mapper;
    EepromMapperSlim _limits_mapper;

    const uint32_t BASE_ADDRESS;    ///> The base address (start of page) for the saved config.

    //Metadata in EEPROM
    uint64_t _target_device;        ///> One-Wire serial number for the configuration saved in EEPROM.
    CRC8_t _combined_crc;           ///> XOR hash of all of the CRCs.

    bool _mapped;                   ///> True when the EEPROM is correctly linked.

    bool _configured;               ///> True when the params structure has been intialized for the target device.    
    uint64_t _configured_device;    ///> One-Wire serial number for the configuration saved in memory.

    CRC8_t hash_crcs() const;

    void save_header();

    void full_save();

public:
    Stepper_Parameters params;

    /**
     * Constructs a stepper config.
     * \param[in]       base_address The address where the Stepper_Save_Params is stored.
     */
    StepperSavedConfigs (const uint32_t base_address);


    /**
     * Loads all configs into memory.
     */
    void load ();

    /**
     * Indicates if the saved data is valid for a given device.
     * \param[in]       serial_number The one-wire serial number for the device.
     * \return true The config saved was mapped to this device.
     * \return false The config saves does not exist, or is not mapped to this device.
     */
    bool is_save_valid (const uint64_t serial_number);

    /**
     * Returns if the EEPROM is memory-mapped.
     * \return true The EEPROM is memory-mapped to the parameters.
     * \return false The EEPROM is not memory-mapped to the parameters.
     */
    bool is_mapped () const;

    /**
     * Indicates if the params tructure has been configured to the passed device serial number.
     * \param[in]       serial_number The one-wire serial number of the connected device.
     * \return true The params structure has been configured and for that device.
     * \return false The params structure cannot be used.
     */
    bool is_params_configured (const uint64_t serial_number) const;

    /**
     * Sent from external code that indicates the the data was configured.
     * \param[in]       serial_number The serial number of the device configured.
     * \note The serial number configured and saved on EEPROM may be different.
     */
    void mark_configured (const uint64_t serial_number);

    /**
     * Gets the device the save structure in EEPROm is targetting.
     * \return uint64_t The serial number of the targeted device.
     */
    uint64_t get_target_device () const;

    /**
     * Gets the device the save structure in memory is targetting.
     * \return uint64_t The serial number for the configure device.
     */
    uint64_t get_configured_device () const;

    /**
     * Invalidates the saved data and write to EEPROM.
     */
    void invalidate ();

    void save_limit_params ();
    void save_soft_limits ();
    void save_home_params ();
    void save_stage_params ();
    void save_jog_params ();
    void save_pid_params ();

};

class StepperSave
{
public:
    // Low-frequency data
    StepperSavedConfigs config;

    // High-frequency data
    Stepper_Store store;

    StepperSave(const uint32_t base_address);
};

#endif



/*****************************************************************************
 * Constants
 *****************************************************************************/



/*****************************************************************************
 * Public Functions
 *****************************************************************************/


#endif /* SRC_SYSTEM_CARDS_STEPPER_STEPPER_SAVES_H_ */
//EOF
