#include "lut-builder/compiler.hh"

#include <string>
#include <stdexcept>
#include <unordered_map>

#include "lut-builder/workers/common.hh"
#include "tinyxml2.h"
#include "lut-builder/workers/stepper_builder.hh"
#include "lut-builder/workers/encoder_builder.hh"
#include "lut-builder/workers/limits_builder.hh"
#include "structure_id.hh"
#include "save_size.hh"
#include "lut-builder/preprocessor.hh"

#include "slot_types.h"

constexpr char DEFAULT_OUTPUT_PATH[] = "output.bin";

void extract_xml_data(std::unordered_map<std::string, std::string>& r_params, const tinyxml2::XMLElement * const p_element, const std::string path);



void extract_xml_data(std::unordered_map<std::string, std::string>& r_params, const tinyxml2::XMLElement * const p_element, const std::string path)
{
    if (p_element->NoChildren())
    {
        // No children, so a parameter node.
        const tinyxml2::XMLAttribute* itr = p_element->FirstAttribute();

        // Parse all of its attributes and add it to r_params
        while (itr != nullptr)
        {
            std::string name = path + p_element->Name();
            if (strcmp(itr->Name(), "value"))
            {
                name = name + "_" + itr->Name();
            }

            r_params[name] = std::string(itr->Value());

            itr = itr->Next();
        }

    } else {
        // Children, so a group node
        const tinyxml2::XMLElement* itr = p_element->FirstChildElement();

        // Recursively call this function on every child
        while (itr != nullptr)
        {
            extract_xml_data(r_params, itr, path + p_element->Name() + "/");
            itr = itr->NextSiblingElement();
        }
    }
}

std::unordered_map<std::string, std::string> extract_xml_data(const tinyxml2::XMLElement * const p_element)
{
    std::unordered_map<std::string, std::string> rt;

    const tinyxml2::XMLElement* itr = p_element->FirstChildElement();

    while (itr != nullptr)
    {
        extract_xml_data(rt, itr, std::string());
        itr = itr->NextSiblingElement();
    }
    return rt;
}


static int get_out_size_helper(tinyxml2::XMLDocument& doc)
{
    int rt = -1;

    std::string name = doc.FirstChildElement()->Name();
    if (name == "Device") {

        tinyxml2::XMLElement * p_signature_node = doc.FirstChildElement()->FirstChildElement("Signature");
        tinyxml2::XMLElement * p_configs_node = doc.FirstChildElement()->FirstChildElement("Configs");

        if (p_signature_node != nullptr && p_configs_node != nullptr)
        {
            lut_manager::device_lut_key_t signature = builders::parse_device_signature(p_signature_node);

            const int BASE = sizeof(signature) + sizeof(CRC8_t);
            switch(signature.running_slot_card)
            {
                case slot_types::ST_Stepper_type:
                case slot_types::ST_HC_Stepper_type:
                case slot_types::High_Current_Stepper_Card_HD:
                case slot_types::ST_Invert_Stepper_BISS_type:
                case slot_types::ST_Invert_Stepper_SSI_type:
                case slot_types::MCM_Stepper_Internal_BISS_L6470:
                case slot_types::MCM_Stepper_Internal_SSI_L6470:
                case slot_types::MCM_Stepper_L6470_MicroDB15:
                case slot_types::MCM_Stepper_LC_HD_DB15:
                    rt = BASE + sizeof(config_signature_t) * STEPPER_CONFIGS_TOTAL;
                break;
                default:
                break;
            }
        }
    } else if (name == "Struct") {

        tinyxml2::XMLElement * p_signature_node = doc.FirstChildElement()->FirstChildElement("Signature");
        tinyxml2::XMLElement * p_data_node = doc.FirstChildElement()->FirstChildElement("Data");

        if (p_signature_node != nullptr && p_data_node != nullptr)
        {
            lut_manager::config_lut_key_t signature = builders::parse_config_signature(p_signature_node);
            const std::size_t CONFIG_SIZE = save_constructor::get_config_size(signature.struct_id);
            rt = CONFIG_SIZE + sizeof(signature) + sizeof(CRC8_t);
        }
    }

    return rt;
}

static bool compile_helper(tinyxml2::XMLDocument& doc, compiled_entry_slim& r_slim)
{
    std::vector<char> serialized_data;
    std::string output_file = DEFAULT_OUTPUT_PATH;

    // Create mapping vector.
    std::string name = doc.FirstChildElement()->Name();
    if (name == "Device") {
        compile_device(doc.RootElement(), r_slim);
    } else if (name == "Struct") {
        compile_config(doc.RootElement(), r_slim);
    } else {
        return false;
    }

    return true;

}

void compile_config(tinyxml2::XMLElement * p_parent, compiled_entry_slim& r_slim)
{
    r_slim.keys.resize(2);
    r_slim.keys[0].resize(sizeof(struct_id_t));
    r_slim.keys[1].resize(sizeof(config_id_t));

    tinyxml2::XMLElement * p_signature_node = p_parent->FirstChildElement("Signature");
    tinyxml2::XMLElement * p_data_node = p_parent->FirstChildElement("Data");

    if (p_signature_node == nullptr)
    {
        throw std::invalid_argument("parent node has no child element \"Signature\"");
    }
    if (p_data_node == nullptr)
    {
        throw std::invalid_argument("parent node has no child element \"Data\"");
    }
    std::vector<char> serialized_data;

    if (p_signature_node != nullptr && p_data_node != nullptr) {
        lut_manager::config_lut_key_t signature = builders::parse_config_signature(p_signature_node);

        switch(static_cast<Struct_ID>(signature.struct_id))
        {
        case Struct_ID::ENCODER:
        {
            const Encoder_Save structure = builders::encoder::build_struct_encoder(p_data_node);
            serialized_data = builders::serialize_config(
                signature,
                reinterpret_cast<const char*>(&structure),
                sizeof(structure)
            );
        }
        break;
        case Struct_ID::LIMIT:
        {
            const Limits_save structure = builders::limits::build_struct_limits(p_data_node);
            serialized_data = builders::serialize_config(
                signature,
                reinterpret_cast<const char*>(&structure),
                sizeof(structure)
            );
        }
        break;
        case Struct_ID::STEPPER_CONFIG:
        {
            const Stepper_Config structure = builders::stepper::build_struct_config(p_data_node);
            serialized_data = builders::serialize_config(
                signature,
                reinterpret_cast<const char*>(&structure),
                sizeof(structure)
            );
        }
        break;
        case Struct_ID::STEPPER_DRIVE:
        {
            const Stepper_drive_params structure = builders::stepper::build_struct_drive(p_data_node);
            serialized_data = builders::serialize_config(
                signature,
                reinterpret_cast<const char*>(&structure),
                sizeof(structure)
            );
        }
        break;
        case Struct_ID::STEPPER_FLAGS:
        {
            const Flags_save structure = builders::stepper::build_struct_flags(p_data_node);
            serialized_data = builders::serialize_config(
                signature,
                reinterpret_cast<const char*>(&structure),
                sizeof(structure)
            );
        }
        break;
        case Struct_ID::STEPPER_HOME:
        {
            const Home_Params structure = builders::stepper::build_struct_home(p_data_node);
            serialized_data = builders::serialize_config(
                signature,
                reinterpret_cast<const char*>(&structure),
                sizeof(structure)
            );
        }
        break;
        case Struct_ID::STEPPER_JOG:
        {
            const Jog_Params structure = builders::stepper::build_struct_jog(p_data_node);
            serialized_data = builders::serialize_config(
                signature,
                reinterpret_cast<const char*>(&structure),
                sizeof(structure)
            );
        }
        break;
        case Struct_ID::STEPPER_PID:
        {
            const Stepper_Pid_Save structure = builders::stepper::build_struct_pid(p_data_node);
            serialized_data = builders::serialize_config(
                signature,
                reinterpret_cast<const char*>(&structure),
                sizeof(structure)
            );
        }
        break;
        case Struct_ID::STEPPER_STORE:
        {
            const Stepper_Store structure = builders::stepper::build_struct_store(p_data_node);
            serialized_data = builders::serialize_config(
                signature,
                reinterpret_cast<const char*>(&structure),
                sizeof(structure)
            );
        }
        break;
        default:
        break;
        }
    }
    if (serialized_data.size() == 0)
    {
        throw std::runtime_error("failed to compile");
    }
    const std::size_t PAYLOAD_SIZE = serialized_data.size() - sizeof(struct_id_t) - sizeof(config_id_t) - sizeof(CRC8_t);
    r_slim.structure.resize(PAYLOAD_SIZE);

    // Copy keys
    memcpy(r_slim.keys[0].data(), &serialized_data.data()[0], sizeof(struct_id_t));
    memcpy(r_slim.keys[1].data(), &serialized_data.data()[sizeof(struct_id_t)], sizeof(config_id_t));

    // Copy data
    memcpy(r_slim.structure.data(), &serialized_data.data()[sizeof(struct_id_t) + sizeof(config_id_t)], r_slim.structure.size());

    // Copy CRC
    memcpy(&r_slim.crc, &serialized_data.data()[serialized_data.size() - 1], sizeof(r_slim.crc));

}

void compile_device(tinyxml2::XMLElement * p_parent, compiled_entry_slim& r_slim)
{
    r_slim.keys.resize(3);
    r_slim.keys[0].resize(sizeof(slot_types));
    r_slim.keys[1].resize(sizeof(slot_types));
    r_slim.keys[2].resize(sizeof(device_id_t));

    tinyxml2::XMLElement * p_signature_node = p_parent->FirstChildElement("Signature");
    tinyxml2::XMLElement * p_configs_node = p_parent->FirstChildElement("Configs");

    std::vector<char> serialized_data;

    if (p_signature_node == nullptr)
    {
        throw std::invalid_argument("parent node has no child element \"Signature\"");
    }
    if (p_configs_node == nullptr)
    {
        throw std::invalid_argument("parent node has no child element \"Configs\"");
    }

    if (p_signature_node != nullptr && p_configs_node != nullptr) {
        lut_manager::device_lut_key_t signature = builders::parse_device_signature(p_signature_node);

        switch (signature.running_slot_card)
        {
        case slot_types::ST_Stepper_type:
        case slot_types::ST_HC_Stepper_type:
        case slot_types::High_Current_Stepper_Card_HD:
        case slot_types::ST_Invert_Stepper_BISS_type:
        case slot_types::ST_Invert_Stepper_SSI_type:
        case slot_types::MCM_Stepper_Internal_BISS_L6470:
        case slot_types::MCM_Stepper_Internal_SSI_L6470:
        case slot_types::MCM_Stepper_L6470_MicroDB15:
        case slot_types::MCM_Stepper_LC_HD_DB15:
        {
            const Stepper_Save_Configs configs = builders::stepper::build_config_list(p_configs_node);
            serialized_data = builders::serialize_device(
                signature,
                reinterpret_cast<const char*>(&configs),
                sizeof(configs)
            );
        }
        break;
        default:
        break;
        }

        if (serialized_data.size() == 0)
        {
            throw std::runtime_error("failed to compile");
        }

        const std::size_t PAYLOAD_SIZE = serialized_data.size() - 2*sizeof(slot_types) - sizeof(device_id_t) - sizeof(CRC8_t);
        r_slim.structure.resize(PAYLOAD_SIZE);

        // Copy keys
        memcpy(r_slim.keys[0].data(), &serialized_data.data()[0], sizeof(slot_types));
        memcpy(r_slim.keys[1].data(), &serialized_data.data()[sizeof(slot_types)], sizeof(slot_types));
        memcpy(r_slim.keys[2].data(), &serialized_data.data()[2*sizeof(slot_types)], sizeof(device_id_t));

        // Copy data
        memcpy(r_slim.structure.data(), &serialized_data.data()[2*sizeof(slot_types) + sizeof(device_id_t)], r_slim.structure.size());

        // Copy CRC
        memcpy(&r_slim.crc, &serialized_data.data()[serialized_data.size() - 1], sizeof(r_slim.crc));
        
    }
}

void compile(compiled_entry_slim& r_slim, const std::filesystem::path& r_path)
{
    tinyxml2::XMLDocument doc;
    const tinyxml2::XMLError ERR = doc.LoadFile(r_path.string().c_str());
    if (ERR != tinyxml2::XMLError::XML_SUCCESS)
    {
        throw std::runtime_error(std::string("XML failed to load.  Error code:  ")
            + std::to_string(static_cast<std::size_t>(ERR)));
    }
    preprocess_xml(doc, r_path);

    const int SIZE = get_out_size_helper(doc);
    if (SIZE != -1) {
        if (!compile_helper(doc, r_slim)) {
            throw std::invalid_argument(std::string("failed to compile"));
        }
    } else {
        throw std::invalid_argument(std::string("failed to get compiled size"));
    }
}

//EOF
