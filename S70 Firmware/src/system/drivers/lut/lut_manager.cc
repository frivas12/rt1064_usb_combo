// lut_manager.cc

/**************************************************************************//**
 * \file lut_manager.cc
 * \author Sean Benish
 *****************************************************************************/

#include "lut_manager.hh"
#include "lut_manager.h"

#include "locked_lut.tcc"
#include "indirect_lut.tcc"

#include "25lc1024.h"
#include "save_constructor.hh"

#include "FreeRTOS.h"
#include "semphr.h"

#include "efs.hh"
#include "efs-cache.hh"

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
// Might want to lock and protect.
static volatile bool s_locked;

static efs::init_file_cache s_config_file(efs::create_id(static_cast<uint8_t>(LUT_ID::CONFIG_LUT), efs::file_types_e::LUT));
static efs::init_file_cache s_device_file(efs::create_id(static_cast<uint8_t>(LUT_ID::DEVICE_LUT), efs::file_types_e::LUT));

static LockedLUT<struct_id_t, config_id_t> s_config_lut(LUT_ID::CONFIG_LUT, &save_constructor::get_config_size);
static IndirectLUT<slot_types, slot_types, device_id_t> s_device_lut(LUT_ID::DEVICE_LUT, &save_constructor::get_slot_save_size);

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 * lut_manager.hh
 *****************************************************************************/

void lut_manager::set_lock(const bool locked)
{
    s_locked = locked;

    if (s_locked)
    {
        // Own the files.
        if (!s_config_file.take_ownership_init())
        {
            s_locked = false;
        }
        if (!s_device_file.take_ownership_init())
        {
            s_locked = false;

        }

        if (s_locked)
        {
            for (slot_nums slot = SLOT_1; slot <= SLOT_8; ++slot)
            {
                device_detect_reset(&slots[slot].device);
            }
        }
    }

    // Release ownership if not all files are owned or asked to be unlocked.
    if (!s_locked)
    {
        // Calling release is file, as it does nothing if the file is not owned.
        s_config_file.release_ownership_init();
        s_device_file.release_ownership_init();
    }
}

bool lut_manager::get_lock()
{
    return s_locked;
}

LUT_ERROR lut_manager::load_data (const LUT_ID lut, const void * const p_signature, void * const p_save)
{
    LUT_ERROR rt;

    switch(lut)
    {
    case LUT_ID::DEVICE_LUT:
    {
        efs::cache_handle&& handle = s_device_file.get_handle(false);
        if (handle.is_valid()) {
            device_lut_key_t key;
            memcpy(&key, p_signature, sizeof(key));
            rt = s_device_lut.load_data(
                handle,
                key.running_slot_card,
                key.connected_device.slot_type,
                key.connected_device.device_id,
                static_cast<char *>(p_save)
            );
        } else {
            rt = LUT_ERROR::MISSING_FILE;
        }
    }
    break;
    case LUT_ID::CONFIG_LUT:
    {
        efs::cache_handle&& handle = s_config_file.get_handle(false);
        if (handle.is_valid()) {
            config_lut_key_t key;
            memcpy(&key, p_signature, sizeof(key));
            rt = s_config_lut.load_data(
                handle,
                key.struct_id,
                key.config_id,
                static_cast<char *>(p_save)
            );
        } else {
            rt = LUT_ERROR::MISSING_FILE;
        }
    }
    break;
    default:
        rt = LUT_ERROR::INVALID_LUT_ID;
    break;
    }

    return rt;
}

void lut_manager::init()
{
    set_lock(true);
}

/*****************************************************************************
 * Public Functions
 * lut_manager.h
 *****************************************************************************/

extern "C" void lut_manager_init(void)
{
    lut_manager::init();
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/


// EOF
