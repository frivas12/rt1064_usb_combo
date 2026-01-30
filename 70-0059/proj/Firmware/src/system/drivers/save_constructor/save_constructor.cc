// save_constructor.cc

/**************************************************************************/ /**
                                                                              * \file save_constructor.cc
                                                                              * \author Sean Benish
                                                                              *****************************************************************************/

#include "save_constructor.hh"

#include "defs.h"
#include "device_detect.h"
#include "structure_id.hh"

#include "defs.hh"
#include "lut.tcc"
#include "lut_manager.hh"

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

/**
 *
 * \param p_custom_entires
 * \param key
 * \return 0 if the entry does not have a custom key.
 */
static uint16_t custom_config_has_key(void *const p_custom_entires,
                                      const uint16_t len,
                                      lut_manager::config_lut_key_t key);

/*****************************************************************************
 * Static Data
 *****************************************************************************/

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/

CRC8_t save_constructor::validate(const device_signature_t saved_signature,
                                  const void *const p_save,
                                  uint32_t save_size) {
    return CRC_split(static_cast<const char *>(p_save), save_size,
                     CRC(reinterpret_cast<const char *>(&saved_signature),
                         sizeof(saved_signature)));
}

uint32_t save_constructor::construct_single(const config_signature_t &signature,
                                            void *const buffer,
                                            void *const custom_entries,
                                            uint16_t custom_entries_size) {
    const lut_manager::config_lut_key_t key{
        .struct_id = signature.struct_id,
        .config_id = signature.config_id,
    };
    const uint16_t OFFSET =
        custom_config_has_key(custom_entries, custom_entries_size, key);
    const uint32_t DATA_SIZE = get_config_size(key.struct_id);
    if (OFFSET != 0) {
        const uint8_t * const LOCATION = reinterpret_cast<const uint8_t*>(custom_entries) + OFFSET;
        memcpy(buffer, LOCATION, DATA_SIZE);
        return DATA_SIZE;
    } else if (key.config_id >= OW_CUSTOM_CONFIG(0)) {
        // Didn't have a custom configuration, but we have a custom id.
        return 0;
    } else {
        return (lut_manager::load_data(LUT_ID::CONFIG_LUT, &key, buffer) ==
                LUT_ERROR::OKAY)
                   ? DATA_SIZE
                   : 0;
    }
}

bool save_constructor::construct(const config_signature_t *p_config_signatures,
                                 uint16_t signature_count, void *const p_buffer,
                                 void *const p_custom_entires,
                                 const uint16_t custom_len) {
    bool rt = true;

    char *p_bytes = static_cast<char *>(p_buffer);

    // Construct from the LUT every entry in config signatures.
    for (uint16_t index = 0; index < signature_count && rt; ++index) {
        const uint32_t increment = construct_single(
            p_config_signatures[index], p_bytes, p_custom_entires, custom_len);
        rt = increment > 0;
        p_bytes += increment;
    }

    return rt;
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

static uint16_t custom_config_has_key(void *const p_custom_entires,
                                      const uint16_t len,
                                      lut_manager::config_lut_key_t key) {
    uint16_t offset = 0;

    if (len > sizeof(One_Wire_Config_Header_t)) {
        // Read first entry to get the header end offset.
        One_Wire_Config_Header_t header;
        memcpy(&header, p_custom_entires, sizeof(header));

        char *p_custom = static_cast<char *>(p_custom_entires);
        const char *const END_HEADER = p_custom + header.index;

        while (p_custom < END_HEADER) {
            // Scan through each header entry until a matching key is found.
            memcpy(&header, p_custom, sizeof(header));
            if (header.signature == key) {
                offset = header.index;
                break;
            }
            p_custom += sizeof(header);
        }
    }

    return offset;
}

// EOF
