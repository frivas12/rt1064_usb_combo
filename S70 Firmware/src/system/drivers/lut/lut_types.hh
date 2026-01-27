// lut_types.hh

#ifndef _SRC_SYSTEM_DRIVERS_LUT_LUT_TYPES_HH_
#define _SRC_SYSTEM_DRIVERS_LUT_LUT_TYPES_HH_

/**************************************************************************//**
 * \file lut_types.hh
 * \author Sean Benish
 * \brief Data type for LUT errors.
 *****************************************************************************/

/// \warning The enumerations here are used for APT commands, so don't change their
/// enumerated value.
enum class LUT_ERROR : unsigned char
{
    OKAY = 0,               ///> No error
    OUT_OF_MEMORY = 1,      ///> Malloc Fail
    MISSING_KEY = 2,
    BAD_STRUCTURE_KEY = 3,
    BAD_DISCRIMINATOR_KEY = 4,
    INVALID_ENTRY = 5,
    MISSING_FILE = 6,           ///> File could not be found
    UNSUPPORTED_VERSION = 7,    ///> File version is not supported
    KEY_SIZE_MISMATCH = 8,      ///> The size of the key is not the same as the object's key(s).
    INVALID_LUT_ID = 9,
    ORDER_VIOLATION = 10,
    HEADER_MISSING = 11,        ///> When the file read to get the header fails
    INDIRECTION_MISMATCH = 12,  ///> When the level of indirection does not match the firmware
};

enum class LUT_ID : unsigned char
{
    DEVICE_LUT = 0,
    CONFIG_LUT = 1,
    VERSION_FILE = 2,
    LENGTH,
    
    RECURSIVE = 0xFF        // A LUT that is inside of a global LUT.
};

LUT_ID& operator++ (LUT_ID &);
LUT_ID operator++ (LUT_ID &, int);

template<typename STRUCTURE_KEY, typename DISCRIMINATOR_KEY>
struct __attribute__((packed)) LUT_Signature
{
    STRUCTURE_KEY str_key;
    DISCRIMINATOR_KEY disc_key;
};

#endif

//EOF
