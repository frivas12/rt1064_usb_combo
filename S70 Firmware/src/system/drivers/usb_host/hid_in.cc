/**
 * @file hid_in.c
 *
 * @brief Functions for HID in reports
 *
 */

#include "hid_in.h"

#include <25lc1024.h>
#include <asf.h>

#include "hid-mapping.hh"
#include "hid_in.hh"
#include "hid_report.h"
#include "integer-serialization.hh"
#include "joystick_map_in.hh"
#include "spi-transfer-handle.hh"
#include "string.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
static constexpr std::size_t HID_MAPPING_IN_SERIALIZED_SIZE = 16;

// The mode is 1 byte in EEPROM, but 4 bytes in APT.
static constexpr std::size_t HID_MAPPING_IN_APT_SIZE = HID_MAPPING_IN_SERIALIZED_SIZE + 3;

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
// static void clear_empty_mapping_data_in(uint8_t port);
/**
 * Serializes an Hid_Mapping_control_in object to the output stream starting at
 * the pointed object. \pre The pointer to p_stream has at least
 * traits::apt_serialized_size_v<Hid_Mapping_control_in> bytes to hold the
 * serialized object. \param p_stream The stream to write the object to. \param
 * p_object The object to serialize.
 */
static void serialize(void* p_stream,
                      const Hid_Mapping_control_in* const p_object,
                      bool apt_eeprom_flag);
/**
 * Deserialized a binary stream into an Hid_Mapping_control_in object.
 * \pre The pointer to the p_stream has at least
 * traits::apt_serialized_size_v<Hid_Mapping_control_in> bytes to read. \param
 * p_dest The object to build from the stream. \param p_stream The binary data
 * stream from which the object will be constructed.
 */
static void deserialize(Hid_Mapping_control_in* const p_dest,
                        const void* p_stream, bool apt_eeprom_flag);

static void serialize(drivers::apt::joystick_map_in::payload_type& payload,
                      const Hid_Mapping_control_in& object);
static void deserialize(
    const drivers::apt::joystick_map_in::payload_type& payload,
    Hid_Mapping_control_in& object);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void serialize(void* p_stream,
                      const Hid_Mapping_control_in* const p_object,
                      bool apt_eeprom_flag) {
    const uint8_t EEPROM_ALIGNED_MODE = static_cast<uint8_t>(p_object->mode);
    uint8_t* byte_stream              = reinterpret_cast<uint8_t*>(p_stream);

    // Static assert to validate that the seralized data size in the
    // implementation is correct.
    static_assert(
        sizeof(p_object->idVendor) + sizeof(p_object->idProduct) +
                sizeof(p_object->modify_control_port) +
                sizeof(p_object->modify_control_ctl_num) +
                sizeof(p_object->destination_slot) +
                sizeof(p_object->destination_bit) +
                sizeof(p_object->destination_port) +
                sizeof(p_object->destination_virtual) +
                sizeof(p_object->speed_modifier) +
                sizeof(p_object->reverse_dir) + sizeof(p_object->dead_band) +
                sizeof(EEPROM_ALIGNED_MODE) +
                sizeof(p_object->control_disable) ==
            HID_MAPPING_IN_SERIALIZED_SIZE,
        "Hid_Mapping_control_in members accessed do not have the same size as "
        "in the type trait");

    memcpy(byte_stream, &p_object->idVendor, sizeof(p_object->idVendor));
    byte_stream += sizeof(p_object->idVendor);
    memcpy(byte_stream, &p_object->idProduct, sizeof(p_object->idProduct));
    byte_stream += sizeof(p_object->idProduct);
    memcpy(byte_stream, &p_object->modify_control_port,
           sizeof(p_object->modify_control_port));
    byte_stream += sizeof(p_object->modify_control_port);
    memcpy(byte_stream, &p_object->modify_control_ctl_num,
           sizeof(p_object->modify_control_ctl_num));
    byte_stream += sizeof(p_object->modify_control_ctl_num);
    memcpy(byte_stream, &p_object->destination_slot,
           sizeof(p_object->destination_slot));
    byte_stream += sizeof(p_object->destination_slot);
    memcpy(byte_stream, &p_object->destination_bit,
           sizeof(p_object->destination_bit));
    byte_stream += sizeof(p_object->destination_bit);
    memcpy(byte_stream, &p_object->destination_port,
           sizeof(p_object->destination_port));
    byte_stream += sizeof(p_object->destination_port);
    memcpy(byte_stream, &p_object->destination_virtual,
           sizeof(p_object->destination_virtual));
    byte_stream += sizeof(p_object->destination_virtual);
    memcpy(byte_stream, &p_object->speed_modifier,
           sizeof(p_object->speed_modifier));
    byte_stream += sizeof(p_object->speed_modifier);
    memcpy(byte_stream, &p_object->reverse_dir, sizeof(p_object->reverse_dir));
    byte_stream += sizeof(p_object->reverse_dir);
    memcpy(byte_stream, &p_object->dead_band, sizeof(p_object->dead_band));
    byte_stream += sizeof(p_object->dead_band);
    if (apt_eeprom_flag) {
        memcpy(byte_stream, &EEPROM_ALIGNED_MODE, sizeof(EEPROM_ALIGNED_MODE));
        byte_stream += sizeof(EEPROM_ALIGNED_MODE);
    } else {
        const uint32_t APT_ALIGNED_MODE = EEPROM_ALIGNED_MODE;
        memcpy(byte_stream, &APT_ALIGNED_MODE, sizeof(APT_ALIGNED_MODE));
        byte_stream += sizeof(APT_ALIGNED_MODE);
    }
    memcpy(byte_stream, &p_object->control_disable,
           sizeof(p_object->control_disable));
    byte_stream += sizeof(p_object->control_disable);
}

static void deserialize(Hid_Mapping_control_in* const p_dest,
                        const void* p_stream, bool apt_eeprom_flag) {
    const uint8_t* byte_stream = reinterpret_cast<const uint8_t*>(p_stream);
    uint8_t eeprom_aligned_mode;

    // Static assert to validate that the deserialized data size in the
    // implementation is correct.
    static_assert(
        sizeof(p_dest->idVendor) + sizeof(p_dest->idProduct) +
                sizeof(p_dest->modify_control_port) +
                sizeof(p_dest->modify_control_ctl_num) +
                sizeof(p_dest->destination_slot) +
                sizeof(p_dest->destination_bit) +
                sizeof(p_dest->destination_port) +
                sizeof(p_dest->destination_virtual) +
                sizeof(p_dest->speed_modifier) + sizeof(p_dest->reverse_dir) +
                sizeof(p_dest->dead_band) + sizeof(eeprom_aligned_mode) +
                sizeof(p_dest->control_disable) ==
            HID_MAPPING_IN_SERIALIZED_SIZE,
        "Hid_Mapping_control_in members accessed do not have the "
        "same size as in the type trait");

    memcpy(&p_dest->idVendor, byte_stream, sizeof(p_dest->idVendor));
    byte_stream += sizeof(p_dest->idVendor);
    memcpy(&p_dest->idProduct, byte_stream, sizeof(p_dest->idProduct));
    byte_stream += sizeof(p_dest->idProduct);
    memcpy(&p_dest->modify_control_port, byte_stream,
           sizeof(p_dest->modify_control_port));
    byte_stream += sizeof(p_dest->modify_control_port);
    memcpy(&p_dest->modify_control_ctl_num, byte_stream,
           sizeof(p_dest->modify_control_ctl_num));
    byte_stream += sizeof(p_dest->modify_control_ctl_num);
    memcpy(&p_dest->destination_slot, byte_stream,
           sizeof(p_dest->destination_slot));
    byte_stream += sizeof(p_dest->destination_slot);
    memcpy(&p_dest->destination_bit, byte_stream,
           sizeof(p_dest->destination_bit));
    byte_stream += sizeof(p_dest->destination_bit);
    memcpy(&p_dest->destination_port, byte_stream,
           sizeof(p_dest->destination_port));
    byte_stream += sizeof(p_dest->destination_port);
    memcpy(&p_dest->destination_virtual, byte_stream,
           sizeof(p_dest->destination_virtual));
    byte_stream += sizeof(p_dest->destination_virtual);
    memcpy(&p_dest->speed_modifier, byte_stream,
           sizeof(p_dest->speed_modifier));
    byte_stream += sizeof(p_dest->speed_modifier);
    memcpy(&p_dest->reverse_dir, byte_stream, sizeof(p_dest->reverse_dir));
    byte_stream += sizeof(p_dest->reverse_dir);
    memcpy(&p_dest->dead_band, byte_stream, sizeof(p_dest->dead_band));
    byte_stream += sizeof(p_dest->dead_band);
    if (apt_eeprom_flag) {
        memcpy(&eeprom_aligned_mode, byte_stream, sizeof(eeprom_aligned_mode));
        byte_stream += sizeof(eeprom_aligned_mode);

        p_dest->mode = static_cast<Control_Modes>(eeprom_aligned_mode);
    } else {
        uint32_t apt_aligned_mode;
        memcpy(&apt_aligned_mode, byte_stream, sizeof(apt_aligned_mode));
        byte_stream += sizeof(apt_aligned_mode);

        p_dest->mode = static_cast<Control_Modes>(apt_aligned_mode);
    }
    memcpy(&p_dest->control_disable, byte_stream,
           sizeof(p_dest->control_disable));
    byte_stream += sizeof(p_dest->control_disable);
}

static void serialize(drivers::apt::joystick_map_in::payload_type& payload,
                      const Hid_Mapping_control_in& object) {
    payload.permitted_vendor_id           = object.idVendor;
    payload.permitted_product_id          = object.idProduct;
    payload.target_port_numbers_bitset    = object.modify_control_port;
    payload.target_control_numbers_bitset = object.modify_control_ctl_num;
    payload.destination_slots_bitset      = object.destination_slot;
    payload.destination_channels_bitset   = object.destination_bit;
    payload.destination_ports_bitset      = object.destination_port;
    payload.destination_virtual_bitset    = object.destination_virtual;
    payload.speed_modifier                = object.speed_modifier;
    payload.reverse_direction             = object.reverse_dir;
    payload.dead_band                     = object.dead_band;
    payload.control_disabled              = object.control_disable;
    payload.control_mode                  = static_cast<uint32_t>(object.mode);
}

static void deserialize(
    const drivers::apt::joystick_map_in::payload_type& payload,
    Hid_Mapping_control_in& object) {
    object.idVendor               = payload.permitted_vendor_id;
    object.idProduct              = payload.permitted_product_id;
    object.modify_control_port    = payload.target_port_numbers_bitset;
    object.modify_control_ctl_num = payload.target_control_numbers_bitset;
    object.destination_slot       = payload.destination_slots_bitset;
    object.destination_bit        = payload.destination_channels_bitset;
    object.destination_port       = payload.destination_ports_bitset;
    object.destination_virtual    = payload.destination_virtual_bitset;
    object.speed_modifier         = payload.speed_modifier;
    object.reverse_dir            = payload.reverse_direction;
    object.dead_band              = payload.dead_band;
    object.control_disable        = payload.control_disabled;
    object.mode = static_cast<Control_Modes>(payload.control_mode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void drivers::usbhid::apt_set_handler(
    const drivers::apt::joystick_map_in::payload_type& payload) {
    if (!service::hid_mapping::is_control_valid(payload.address(),
                                                payload.control_number)) {
        return;
    }

    using handle_t  = service::hid_mapping::address_handle;
    handle_t handle = handle_t::create(payload.port_number + 1);

    Hid_Mapping_control_in in;
    deserialize(payload, in);

    handle.set_apt(in, payload.control_number);

    // (sbenish) As of 2024-10-25, disabling a control
    // over APT will not cause it to disconnect until the control logic
    // is reset.
}

drivers::apt::joystick_map_in::payload_type drivers::usbhid::apt_get_handler(
    const drivers::apt::joystick_map_in::request_type& request) {
    drivers::apt::joystick_map_in::payload_type rt;

    if (service::hid_mapping::is_control_valid(request.address(),
                                               request.control_number)) {
        Hid_Mapping_control_in in;
        serialize(rt, in);
    } else {
        memset(&rt, 0, sizeof(rt));
        rt.control_disabled = true;
    }

    return rt;
}

extern "C" void hid_mapping_control_in_persist(uint8_t port) {
    if (!service::hid_mapping::is_address_valid(port + 1)) {
        return;
    }

    using handle_t  = service::hid_mapping::address_handle;
    handle_t handle = handle_t::create(port + 1);

    auto spi = drivers::spi::handle_factory();
    handle.store_in_to_eeprom(spi);
}

extern "C" void* hid_mapping_control_in_serialize_apt(
    const Hid_Mapping_control_in* obj, void* dest, size_t length) {
    static constexpr std::size_t REQUIRED = HID_MAPPING_IN_APT_SIZE;
    if (length < REQUIRED) {
        return dest;
    }

    serialize(dest, obj, false);

    return static_cast<uint8_t*>(dest) + REQUIRED;
}
extern "C" const void* hid_mapping_control_in_deserialize_apt(
    Hid_Mapping_control_in* obj, const void* src, size_t length) {
    static constexpr std::size_t REQUIRED = HID_MAPPING_IN_APT_SIZE;
    if (length < REQUIRED) {
        return src;
    }

    deserialize(obj, src, false);

    return static_cast<const uint8_t*>(src) + REQUIRED;
}

extern "C" void* hid_mapping_control_in_serialize_eeprom(
    const Hid_Mapping_control_in* obj, void* dest, size_t length) {
    constexpr std::size_t REQUIRED = HID_MAPPING_IN_SERIALIZED_SIZE;
    if (length < REQUIRED) {
        return dest;
    }

    serialize(dest, obj, true);

    return static_cast<uint8_t*>(dest) + REQUIRED;
}
extern "C" const void* hid_mapping_control_in_deserialize_eeprom(
    Hid_Mapping_control_in* obj, const void* src, size_t length) {
    constexpr std::size_t REQUIRED = HID_MAPPING_IN_SERIALIZED_SIZE;
    if (length < REQUIRED) {
        return src;
    }

    deserialize(obj, src, true);

    return static_cast<const uint8_t*>(src) + REQUIRED;
}

extern "C" bool control_mode_is_mutable(Control_Modes mode) {
    switch (mode) {
    // Most of these controls are commands applied to a
    // card destination.
    // Therefore, these controls can be mutated.
    case CTL_AXIS:
    case CTL_BTN_JOG_CW:
    case CTL_BTN_JOG_CCW:
    case CTL_BTN_POS_TOGGLE:
    case CTL_BTN_HOME:
    case CTL_BTN_POS_1:
    case CTL_BTN_POS_2:
    case CTL_BTN_POS_3:
    case CTL_BTN_POS_4:
    case CTL_BTN_POS_5:
    case CTL_BTN_POS_6:
    case CTL_BTN_POS_7:
    case CTL_BTN_POS_8:
    case CTL_BTN_POS_9:
    case CTL_BTN_POS_10:
    case CTL_BTN_TOGGLE_DISABLE_DEST:
    case CTL_BTN_DISABLE_DEST:
        return true;

    // Most of these control modes are mutators.
    // We don't allow mutations of mutators.
    case CTL_BTN_SELECT:
    case CTL_BTN_SELECT_TOGGLE:
    case CTL_BTN_TOGGLE_DISABLE_MOD:
    case CTL_BTN_DISABLE_MOD:
    case CTL_BTN_SPEED:
    case CTL_BTN_SPEED_TOGGLE:
    default:
        return false;
    }
}

extern "C" bool control_mode_is_mutating(Control_Modes mode) {
    switch (mode) {
    // Most of these controls are commands applied to a
    // card destination.
    // Therefore, these controls can be mutated.
    case CTL_AXIS:
    case CTL_BTN_JOG_CW:
    case CTL_BTN_JOG_CCW:
    case CTL_BTN_POS_TOGGLE:
    case CTL_BTN_HOME:
    case CTL_BTN_POS_1:
    case CTL_BTN_POS_2:
    case CTL_BTN_POS_3:
    case CTL_BTN_POS_4:
    case CTL_BTN_POS_5:
    case CTL_BTN_POS_6:
    case CTL_BTN_POS_7:
    case CTL_BTN_POS_8:
    case CTL_BTN_POS_9:
    case CTL_BTN_POS_10:
    case CTL_BTN_TOGGLE_DISABLE_DEST:
    case CTL_BTN_DISABLE_DEST:
        return false;

    // Most of these control modes are mutators.
    // We don't allow mutations of mutators.
    case CTL_BTN_SELECT:
    case CTL_BTN_SELECT_TOGGLE:
    case CTL_BTN_TOGGLE_DISABLE_MOD:
    case CTL_BTN_DISABLE_MOD:
    case CTL_BTN_SPEED:
    case CTL_BTN_SPEED_TOGGLE:
    default:
        return true;
    }
}
