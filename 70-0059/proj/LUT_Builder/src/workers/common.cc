#include "lut-builder/workers/common.hh"
#include <string>

static CRC8_t crc_split(const char * p_data, const uint32_t length, CRC8_t crc);

CRC8_t builders::CRC(const char * p_data, const uint32_t length)
{
    return crc_split(p_data, length, 0);
}


std::vector<char> builders::serialize_device(const lut_manager::device_lut_key_t signature, const char * p_config_list, const int config_list_size)
{
    std::vector<char> rt;
    rt.reserve((std::size_t)config_list_size + sizeof(device_signature_t) + sizeof(slot_types) + sizeof(CRC8_t));

    const char * p_stream = reinterpret_cast<const char*>(&signature.running_slot_card);
    for (int i = 0; i < sizeof(signature.running_slot_card); ++i)
    {
        rt.push_back(p_stream[i]);
    }

    p_stream = reinterpret_cast<const char*>(&signature.connected_device);
    for (int i = 0; i < sizeof(signature.connected_device); ++i)
    {
        rt.push_back(p_stream[i]);
    }

    p_stream = p_config_list;
    for (int i = 0; i < config_list_size; ++i)
    {
        rt.push_back(p_stream[i]);
    }

    CRC8_t CRC_PART = crc_split(p_config_list, (std::size_t)config_list_size, 
        crc_split(reinterpret_cast<const char*>(&signature.connected_device), sizeof(signature.connected_device),
            CRC(reinterpret_cast<const char*>(&signature.running_slot_card), sizeof(signature.running_slot_card))
        )
    );
    rt.push_back(static_cast<char>(CRC_PART));

    return rt;
}

std::vector<char> builders::serialize_config(const lut_manager::config_lut_key_t signature, const char * p_config, const int config_size)
{
    std::vector<char> rt;
    rt.reserve((std::size_t)config_size + sizeof(config_signature_t) + sizeof(CRC8_t));

    const char * p_stream = reinterpret_cast<const char*>(&signature);
    for (int i = 0; i < sizeof(signature); ++i)
    {
        rt.push_back(p_stream[i]);
    }

    p_stream = p_config;
    for (int i = 0; i < config_size; ++i)
    {
        rt.push_back(p_stream[i]);
    }

    CRC8_t CRC_PART = crc_split(p_config, (std::size_t)config_size, 
        CRC(reinterpret_cast<const char*>(&signature), sizeof(signature))
    );
    rt.push_back(static_cast<char>(CRC_PART));

    return rt;
}

lut_manager::device_lut_key_t builders::parse_device_signature(tinyxml2::XMLElement * const p_signature)
{
    lut_manager::device_lut_key_t rt;

    tinyxml2::XMLElement * itr = p_signature->FirstChildElement();

    while (itr != nullptr)
    {
        const std::string name = itr->Name();

        if (name == "RunningSlotType") {
            rt.running_slot_card = static_cast<slot_types>(itr->FindAttribute("value")->UnsignedValue());
        } else if (name == "DefaultSlotType") {
            rt.connected_device.slot_type = static_cast<slot_types>(itr->FindAttribute("value")->UnsignedValue());
        } else if (name == "DeviceID") {
            rt.connected_device.device_id = static_cast<device_id_t>(itr->FindAttribute("value")->UnsignedValue());
        }

        itr = itr->NextSiblingElement();
    }

    return rt;
}

lut_manager::config_lut_key_t builders::parse_config_signature(tinyxml2::XMLElement * const p_signature)
{
    lut_manager::config_lut_key_t rt;

    tinyxml2::XMLElement * itr = p_signature->FirstChildElement();

    while (itr != nullptr)
    {
        const std::string name = itr->Name();

        if (name == "StructID") {
            rt.struct_id = static_cast<struct_id_t>(itr->FindAttribute("value")->UnsignedValue());
        } else if (name == "ConfigID") {
            rt.config_id = static_cast<config_id_t>(itr->FindAttribute("value")->UnsignedValue());
        }

        itr = itr->NextSiblingElement();
    }

    return rt;
}


static CRC8_t crc_split(const char * p_data, const uint32_t length, CRC8_t crc)
{
    static const uint8_t POLY = 0x31;

    for(uint32_t i = 0; i < length; ++i)
    {
        crc ^= p_data[i];
        for (uint8_t j = 0; j < 8; ++j)
        {
            if ((crc & 0x80) == 0)
            {
                crc <<= 1;
            } else {
                crc = (uint8_t)((crc << 1) ^ POLY);
            }
        }
    }

    return crc;
}

//EOF
