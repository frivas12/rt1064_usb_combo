/**
 * @file hid_out.c
 *
 * @brief Functions for USB device task out data
 *
 * TODO Only slot sources are implemented not bits, ports, nor virtual
 *
 */
#include <asf.h>
#include "hid_out.h"
#include "usb_device.h"
#include <25lc1024.h>
#include <cstdlib>
#include "string.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
static constexpr std::size_t HID_MAP_OUT_SERIALIZED_SIZE = 13;

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/
/**
 * Serializes an Hid_Mapping_control_out object to the output stream starting at the pointed object.
 * \pre The pointer to p_stream has at least traits::apt_serialized_size_v<Hid_Mapping_control_out> bytes to hold the serialized object.
 * \param p_stream The stream to write the object to.
 * \param p_object The object to serialize.
 */
static void serialize(void * p_stream, const Hid_Mapping_control_out * const p_object);
/**
 * Deserialized a binary stream into an Hid_Mapping_control_out object.
 * \pre The pointer to the p_stream has at least traits::apt_serialized_size_v<Hid_Mapping_control_out> bytes to read.
 * \param p_dest The object to build from the stream.
 * \param p_stream The binary data stream from which the object will be constructed.
 */
static void deserialize(Hid_Mapping_control_out * p_dest, const void * p_stream);

/****************************************************************************
 * Interrupt Handler
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void serialize(void * p_stream, const Hid_Mapping_control_out * const p_object)
{
    uint8_t * byte_stream = reinterpret_cast<uint8_t*>(p_stream);
    const uint8_t BYTE_MODE = static_cast<uint8_t>(p_object->mode);
    const uint8_t BYTE_COLOR_1_ID = static_cast<uint8_t>(p_object->color_1_id);
    const uint8_t BYTE_COLOR_2_ID = static_cast<uint8_t>(p_object->color_2_id);
    const uint8_t BYTE_COLOR_3_ID = static_cast<uint8_t>(p_object->color_3_id);

    // Static assert to validate that the seralized data size in the implementation is correct.
    static_assert(
          sizeof(p_object->type)
        + sizeof(p_object->idVendor)
        + sizeof(p_object->idProduct)
        + sizeof(BYTE_MODE)
        + sizeof(BYTE_COLOR_1_ID)
        + sizeof(BYTE_COLOR_2_ID)
        + sizeof(BYTE_COLOR_3_ID)
        + sizeof(p_object->source_slots)
        + sizeof(p_object->source_bit)
        + sizeof(p_object->source_port)
        + sizeof(p_object->source_virtual)
        == HID_MAP_OUT_SERIALIZED_SIZE,
        "Hid_Mapping_control_out members accessed do not have the same size as in the type trait"
    );

    memcpy(byte_stream, &p_object->type,            sizeof(p_object->type)          );  byte_stream += sizeof(p_object->type);
    memcpy(byte_stream, &p_object->idVendor,        sizeof(p_object->idVendor)      );  byte_stream += sizeof(p_object->idVendor);
    memcpy(byte_stream, &p_object->idProduct,       sizeof(p_object->idProduct)     );  byte_stream += sizeof(p_object->idProduct);
    memcpy(byte_stream, &BYTE_MODE,                 sizeof(BYTE_MODE)               );  byte_stream += sizeof(BYTE_MODE);
    memcpy(byte_stream, &BYTE_COLOR_1_ID,           sizeof(BYTE_COLOR_1_ID)         );  byte_stream += sizeof(BYTE_COLOR_1_ID);
    memcpy(byte_stream, &BYTE_COLOR_2_ID,           sizeof(BYTE_COLOR_2_ID)         );  byte_stream += sizeof(BYTE_COLOR_2_ID);
    memcpy(byte_stream, &BYTE_COLOR_3_ID,           sizeof(BYTE_COLOR_3_ID)         );  byte_stream += sizeof(BYTE_COLOR_3_ID);
    memcpy(byte_stream, &p_object->source_slots,    sizeof(p_object->source_slots)  );  byte_stream += sizeof(p_object->source_slots);
    memcpy(byte_stream, &p_object->source_bit,      sizeof(p_object->source_bit)    );  byte_stream += sizeof(p_object->source_bit);
    memcpy(byte_stream, &p_object->source_port,     sizeof(p_object->source_port)   );  byte_stream += sizeof(p_object->source_port);
    memcpy(byte_stream, &p_object->source_virtual,  sizeof(p_object->source_virtual));  byte_stream += sizeof(p_object->source_virtual);
}
static void deserialize(Hid_Mapping_control_out * p_dest, const void * p_stream)
{
    const uint8_t * byte_stream = reinterpret_cast<const uint8_t*>(p_stream);
    uint8_t BYTE_MODE;
    uint8_t BYTE_COLOR_1_ID;
    uint8_t BYTE_COLOR_2_ID;
    uint8_t BYTE_COLOR_3_ID;

    // Static assert to validate that the seralized data size in the implementation is correct.
    static_assert(
          sizeof(p_dest->type)
        + sizeof(p_dest->idVendor)
        + sizeof(p_dest->idProduct)
        + sizeof(BYTE_MODE)
        + sizeof(BYTE_COLOR_1_ID)
        + sizeof(BYTE_COLOR_2_ID)
        + sizeof(BYTE_COLOR_3_ID)
        + sizeof(p_dest->source_slots)
        + sizeof(p_dest->source_bit)
        + sizeof(p_dest->source_port)
        + sizeof(p_dest->source_virtual)
        == HID_MAP_OUT_SERIALIZED_SIZE,
        "Hid_Mapping_control_out members accessed do not have the same size as in the type trait"
    );

    memcpy(&p_dest->type, byte_stream,              sizeof(p_dest->type)          );    byte_stream += sizeof(p_dest->type);
    memcpy(&p_dest->idVendor, byte_stream,          sizeof(p_dest->idVendor)      );    byte_stream += sizeof(p_dest->idVendor);
    memcpy(&p_dest->idProduct, byte_stream,         sizeof(p_dest->idProduct)     );    byte_stream += sizeof(p_dest->idProduct);
    memcpy(&BYTE_MODE, byte_stream,                 sizeof(BYTE_MODE)             );    byte_stream += sizeof(BYTE_MODE);
    memcpy(&BYTE_COLOR_1_ID, byte_stream,           sizeof(BYTE_COLOR_1_ID)       );    byte_stream += sizeof(BYTE_COLOR_1_ID);
    memcpy(&BYTE_COLOR_2_ID, byte_stream,           sizeof(BYTE_COLOR_2_ID)       );    byte_stream += sizeof(BYTE_COLOR_2_ID);
    memcpy(&BYTE_COLOR_3_ID, byte_stream,           sizeof(BYTE_COLOR_3_ID)       );    byte_stream += sizeof(BYTE_COLOR_3_ID);
    memcpy(&p_dest->source_slots, byte_stream,      sizeof(p_dest->source_slots)  );    byte_stream += sizeof(p_dest->source_slots);
    memcpy(&p_dest->source_bit, byte_stream,        sizeof(p_dest->source_bit)    );    byte_stream += sizeof(p_dest->source_bit);
    memcpy(&p_dest->source_port, byte_stream,       sizeof(p_dest->source_port)   );    byte_stream += sizeof(p_dest->source_port);
    memcpy(&p_dest->source_virtual, byte_stream,    sizeof(p_dest->source_virtual));    byte_stream += sizeof(p_dest->source_virtual);

    p_dest->mode = static_cast<Led_modes>(BYTE_MODE);
    p_dest->color_1_id = static_cast<Led_color_id>(BYTE_COLOR_1_ID);
    p_dest->color_2_id = static_cast<Led_color_id>(BYTE_COLOR_2_ID);
    p_dest->color_3_id = static_cast<Led_color_id>(BYTE_COLOR_3_ID);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
extern "C" void * hid_mapping_control_out_serialize(const Hid_Mapping_control_out * obj, void * dest, size_t length) {
    if (length < HID_MAP_OUT_SERIALIZED_SIZE)
    { return dest; }

    serialize(dest, obj);
    return static_cast<uint8_t*>(dest) + 13;
}

extern "C" const void * hid_mapping_control_out_deserialize(Hid_Mapping_control_out * obj, const void * src, size_t length) {
    if (length < HID_MAP_OUT_SERIALIZED_SIZE)
    { return src; }

    deserialize(obj, src);
    return static_cast<const uint8_t*>(src) + 13;
}
