// stepper_saves.cc

/**************************************************************************//**
 * \file stepper_saves.cc
 * \author Sean Benish
 *****************************************************************************/

#include "stepper_saves.h"

#include <stddef.h>

#include "25lc1024.h"

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Macros
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/

/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/

/*****************************************************************************
 * Static Data
 *****************************************************************************/

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

StepperSavedConfigs::StepperSavedConfigs(const uint32_t base_address) :
    BASE_ADDRESS(base_address), _target_device(0xFFFFFFFFFFFFFFFF), _mapped(false),
    _configured(false)
{
}

// MARK:  SPI Mutex Required
void StepperSavedConfigs::load()
{
    eeprom_25LC1024_read(BASE_ADDRESS, sizeof(uint64_t), reinterpret_cast<uint8_t*>(&_target_device));
    eeprom_25LC1024_read(BASE_ADDRESS + sizeof(uint64_t), sizeof(CRC8_t),reinterpret_cast<uint8_t*>( &_combined_crc));

    _config_mapper.load(BASE_ADDRESS + CONFIG_OFFSET, sizeof(Stepper_Config), &params.config);
    _drive_mapper.load(BASE_ADDRESS + DRIVE_OFFSET, sizeof(Stepper_drive_params), &params.drive);
    _flags_mapper.load(BASE_ADDRESS + FLAGS_OFFSET, sizeof(Flags_save), &params.flags);
    _home_mapper.load(BASE_ADDRESS + HOME_OFFSET, sizeof(Home_Params), &params.home);
    _jog_mapper.load(BASE_ADDRESS + JOG_OFFSET, sizeof(Jog_Params), &params.jog);
    _pid_mapper.load(BASE_ADDRESS + PID_OFFSET, sizeof(Stepper_Pid_Save), &params.pid);
    _encoder_mapper.load(BASE_ADDRESS + ENCODER_OFFSET, sizeof(Encoder_Save), &params.encoder);
    _limits_mapper.load(BASE_ADDRESS + LIMITS_OFFSET, sizeof(Limits_save), &params.limits);

    // We don't know if the data is valid, so we mark it the params as unconfigured.
    _configured = false;
}

bool StepperSavedConfigs::is_mapped() const
{
    return _mapped;
}

bool StepperSavedConfigs::is_save_valid(const uint64_t serial_number)
{
    const bool VALID = serial_number == _target_device && hash_crcs() == _combined_crc;
    _mapped = VALID;
    return VALID;
}

bool StepperSavedConfigs::is_params_configured(const uint64_t serial_number) const
{
    return _configured && serial_number == _configured_device;
}

void StepperSavedConfigs::mark_configured(const uint64_t serial_number)
{
    _configured_device = serial_number;
    _configured = true;

    // Invalidate the mapping if the configured device differs from the target device.
    _mapped = _mapped && _configured_device == _target_device;
}

uint64_t StepperSavedConfigs::get_target_device() const
{
    return _target_device;
}

uint64_t StepperSavedConfigs::get_configured_device() const
{
    return _configured_device;
}

// MARK:  SPI Mutex Required
void StepperSavedConfigs::invalidate()
{
    _target_device = 0x0FFFFFFFFFFFFFFF;
    _configured = false;
    save_header();
}

// MARK:  SPI Mutex Required
void StepperSavedConfigs::save_limit_params()
{
    // Fully saved if the parameter structure is not mapped.
    if (!_mapped)
    {
        full_save();
        return;
    }

    // Otherwise, map the limits.
    // Clone the EEPROM's limits into memory (switch memory target).
    Limits_save tmp;
    _limits_mapper.load(BASE_ADDRESS + LIMITS_OFFSET, sizeof(Limits_save), &tmp);
    const CRC8_t OLD = _limits_mapper.get_eeprom_crc();

    // Set the hard limits and limits parameters from memory.
    tmp.ccw_hard_limit = params.limits.ccw_hard_limit;
    tmp.cw_hard_limit = params.limits.cw_hard_limit;
    tmp.limit_mode = params.limits.limit_mode;

    // Save the LIMITS to EEPROM memory.
    _limits_mapper.save(BASE_ADDRESS + LIMITS_OFFSET, sizeof(Limits_save), &tmp);

    // Quick-Calculate the new crc hash.
    const CRC8_t NEW = _limits_mapper.get_eeprom_crc();
    _combined_crc = (_combined_crc ^ OLD) ^ NEW;
    save_header();
}

// MARK:  SPI Mutex Required
void StepperSavedConfigs::save_soft_limits()
{
    // Fully saved if the parameter structure is not mapped.
    if (!_mapped)
    {
        full_save();
        return;
    }

    // Otherwise, map the limits.
    Limits_save tmp;
    _limits_mapper.load(BASE_ADDRESS + LIMITS_OFFSET, sizeof(Limits_save), &tmp);
    const CRC8_t OLD = _limits_mapper.get_eeprom_crc();
    
    // Set the soft limits parameters from memory.
    tmp.ccw_soft_limit = params.limits.ccw_soft_limit;
    tmp.cw_soft_limit = params.limits.cw_soft_limit;

    // Save the soft limits to EEPROM.
    _limits_mapper.save(BASE_ADDRESS + LIMITS_OFFSET, sizeof(Limits_save), &tmp);

    // Quick-Calculate the new crc hash.
    const CRC8_t NEW = CRC(reinterpret_cast<char*>(&tmp), sizeof(tmp));
    _combined_crc = (_combined_crc ^ OLD) ^ NEW;
    save_header();
}
// MARK:  SPI Mutex Required
void StepperSavedConfigs::save_home_params()
{
    // Fully saved if the parameter structure is not mapped.
    if (!_mapped)
    {
        full_save();
        return;
    }

    // Otherwise, map the home.
    const CRC8_t OLD = _home_mapper.get_eeprom_crc();
    _home_mapper.save(BASE_ADDRESS + HOME_OFFSET, sizeof(Home_Params), &params.home);
    const CRC8_t NEW = _home_mapper.get_eeprom_crc();
    _combined_crc = (_combined_crc ^ OLD) ^ NEW;

    save_header();
}
// MARK:  SPI Mutex Required
void StepperSavedConfigs::save_stage_params()
{
    // Fully saved if the parameter structure is not mapped.
    if (!_mapped)
    {
        full_save();
        return;
    }

    // Otherwise, map the stage params.
    const CRC8_t OLD = _config_mapper.get_eeprom_crc() ^
        _drive_mapper.get_eeprom_crc() ^
        _flags_mapper.get_eeprom_crc() ^
        _encoder_mapper.get_eeprom_crc();

    _config_mapper.save(BASE_ADDRESS + CONFIG_OFFSET, sizeof(Stepper_Config), &params.config);
    _drive_mapper.save(BASE_ADDRESS + DRIVE_OFFSET, sizeof(Stepper_drive_params), &params.drive);
    _flags_mapper.save(BASE_ADDRESS + FLAGS_OFFSET, sizeof(Flags_save), &params.flags);
    _encoder_mapper.save(BASE_ADDRESS + ENCODER_OFFSET, sizeof(Encoder_Save), &params.encoder);

    const CRC8_t NEW = _config_mapper.get_eeprom_crc() ^
        _drive_mapper.get_eeprom_crc() ^
        _flags_mapper.get_eeprom_crc() ^
        _encoder_mapper.get_eeprom_crc();

    _combined_crc = (_combined_crc ^ OLD) ^ NEW;
    save_header();
}
// MARK:  SPI Mutex Required
void StepperSavedConfigs::save_jog_params()
{
    // Fully saved if the parameter structure is not mapped.
    if (!_mapped)
    {
        full_save();
        return;
    }

    // Otherwise, map the jog params.
    const CRC8_t OLD = _jog_mapper.get_eeprom_crc();

    _jog_mapper.save(BASE_ADDRESS + JOG_OFFSET, sizeof(Jog_Params), &params.jog);

    const CRC8_t NEW = _jog_mapper.get_eeprom_crc();

    _combined_crc = (_combined_crc ^ OLD) ^ NEW;
    save_header();
}
// MARK:  SPI Mutex Required
void StepperSavedConfigs::save_pid_params()
{
    // Fully saved if the parameter structure is not mapped.
    if (!_mapped)
    {
        full_save();
        return;
    }

    // Otherwise, map the pid params.

    const CRC8_t OLD = _pid_mapper.get_eeprom_crc();

    _pid_mapper.save(BASE_ADDRESS + PID_OFFSET, sizeof(Stepper_Pid_Save), &params.pid);

    const CRC8_t NEW = _pid_mapper.get_eeprom_crc();

    _combined_crc = (_combined_crc ^ OLD) ^ NEW;
    save_header();
}


/*****************************************************************************
 * Private Functions
 *****************************************************************************/

CRC8_t StepperSavedConfigs::hash_crcs() const
{
    return
        _config_mapper.get_eeprom_crc() ^
        _drive_mapper.get_eeprom_crc() ^
        _flags_mapper.get_eeprom_crc() ^
        _home_mapper.get_eeprom_crc() ^
        _jog_mapper.get_eeprom_crc() ^
        _pid_mapper.get_eeprom_crc() ^
        _encoder_mapper.get_eeprom_crc() ^
        _limits_mapper.get_eeprom_crc();
}

// MARK:  SPI Mutex Required
void StepperSavedConfigs::save_header()
{
    eeprom_25LC1024_write(BASE_ADDRESS, sizeof(uint64_t), reinterpret_cast<uint8_t*>(&_target_device));
    eeprom_25LC1024_write(BASE_ADDRESS + sizeof(uint64_t), sizeof(CRC8_t),reinterpret_cast<uint8_t*>( &_combined_crc));
}

// MARK:  SPI Mutex Required
void StepperSavedConfigs::full_save()
{
    // * Assertion: The configured flag is set.
    _config_mapper.save(BASE_ADDRESS + CONFIG_OFFSET, sizeof(Stepper_Config), &params.config);
    _drive_mapper.save(BASE_ADDRESS + DRIVE_OFFSET, sizeof(Stepper_drive_params), &params.drive);
    _flags_mapper.save(BASE_ADDRESS + FLAGS_OFFSET, sizeof(Flags_save), &params.flags);
    _home_mapper.save(BASE_ADDRESS + HOME_OFFSET, sizeof(Home_Params), &params.home);
    _jog_mapper.save(BASE_ADDRESS + JOG_OFFSET, sizeof(Jog_Params), &params.jog);
    _pid_mapper.save(BASE_ADDRESS + PID_OFFSET, sizeof(Stepper_Pid_Save), &params.pid);
    _encoder_mapper.save(BASE_ADDRESS + ENCODER_OFFSET, sizeof(Encoder_Save), &params.encoder);
    _limits_mapper.save(BASE_ADDRESS + LIMITS_OFFSET, sizeof(Limits_save), &params.limits);

    _target_device = _configured_device;
    _combined_crc = hash_crcs();
    save_header();

    _mapped = true;
}

StepperSave::StepperSave(const uint32_t base_address)
    : config(static_cast<std::size_t>(base_address))
{}

// EOF
