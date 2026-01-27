// config_save.tcc

/**************************************************************************//**
 * \file config_save.tcc
 * \author Sean Benish
 * \brief Implements the ConfigSave template for verified save data.
 *****************************************************************************/

#ifndef SRC_SYSTEM_DRIVERS_SAVE_CONSTRUCTOR_CONFIG_SAVE_TCC
#define SRC_SYSTEM_DRIVERS_SAVE_CONSTRUCTOR_CONFIG_SAVE_TCC

#include "defs.hh"
#include "save_constructor.hh"

#ifndef _DESKTOP_BUILD
#include "25lc1024.h"
#else
#define eeprom_25LC1024_write(a, b, c)
#define eeprom_25LC1024_read(a, b, c)
#endif

/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Macros
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/

/**
 * This class indicates if
 * \tparam T
 */
template<typename T>
struct __attribute__((packed)) ConfigSave
{
public:
    device_signature_t signature;
    T saved_params;
private:
    CRC8_t crc;
public:


    /**
     * \bug If erased data is checked (0xFF) and (sizeof(signature) +
     * sizeof(T)) % 127 == 1, then it will validate erased data!
     * \return true The signature and saved data match the saved crc.
     */
    inline bool valid() const
    {
        return save_constructor::check_valid(signature, &saved_params, sizeof(saved_params), crc);
    }

    /**
     * Updates the saved CRC to validate the current signature and data.
     */
    inline void validate()
    {
        crc = save_constructor::validate(signature, &saved_params, sizeof(saved_params));
    }


    /**
     * Guarantees that the saved CRC does NOT match the current signature and data,
     * then writes that data into storage.
     * \param[in]       base_address The starting address in storage that should be written to.
     */
    inline void invalidate(const uint32_t base_address)
    {
        validate();
        ++crc;

        eeprom_25LC1024_write(
            base_address,
            sizeof(ConfigSave<T>),
            reinterpret_cast<uint8_t*>(this)
        );
    }

    /**
     * Writes the data into storage.
     * \param[in]       base_address The starting address in storage that should be written to.
     */
    inline void write_eeprom(const uint32_t base_address)
    {

        validate();

        eeprom_25LC1024_write(
            base_address,
            sizeof(ConfigSave<T>),
            reinterpret_cast<uint8_t*>(this)
        );
    }

    /**
     * Reads the data from storage.
     * \param[in]       base_address The starting address in storage that should be read from.
     */
    inline void read_eeprom(const uint32_t base_address)
    {
        eeprom_25LC1024_read(
            base_address,
            sizeof(ConfigSave<T>),
            reinterpret_cast<uint8_t*>(this)
        );
    }
};

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

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

#endif

// EOF
