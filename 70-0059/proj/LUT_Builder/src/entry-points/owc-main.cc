// main.cc

/**************************************************************************//**
 * \file main.cc
 * \author Sean Benish
 *****************************************************************************/
#include <iostream>
#include <fstream>
#include <cstring>

#include "defs.h"
#include "tinyxml2.h"

#include "lut-builder/cmd-line-parser.hh"
#include "lut-builder/preprocessor.hh"
#include "lut-builder/compiler.hh"

/*****************************************************************************
 * Constants
 *****************************************************************************/
constexpr one_wire_versions_e DEFAULT_VERSION =
    one_wire_versions_e::VERSION_1;

/*****************************************************************************
 * Macros
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/
struct ow_config_entry
{
    config_signature_t sig;
    // uint16_t index;
    std::vector<uint8_t> payload;

    ow_config_entry() = default;
    ow_config_entry(compiled_entry_slim& r_slim)
        : payload(r_slim.structure)
    {
        memcpy(&sig.struct_id, r_slim.keys[0].data(), sizeof(sig.struct_id));
        memcpy(&sig.config_id, r_slim.keys[1].data(), sizeof(sig.config_id));
    }
};

struct ow_device_entry
{
    uint16_t length;
    slot_types slot_card_in_use;
    std::vector<config_signature_t> device_entries;

    ow_device_entry() = default;
    ow_device_entry(compiled_entry_slim& r_slim)
        : length(sizeof(length) + sizeof(slot_card_in_use) + sizeof(r_slim.structure.size()))
    {
        memcpy(&slot_card_in_use, r_slim.keys[0].data(), sizeof(slot_card_in_use));
        device_entries.reserve(r_slim.keys.size() / sizeof(config_signature_t));
        for (std::size_t i = 0; i < r_slim.structure.size(); i += sizeof(config_signature_t))
        {
            config_signature_t sig;
            memcpy(&sig, r_slim.structure.data() + i, sizeof(sig));
            device_entries.push_back(sig);
        }
    }

    std::vector<uint8_t> serialize() const
    {
        std::vector<uint8_t> rt
        (
            sizeof(length) + sizeof(slot_card_in_use) + sizeof(config_signature_t)*device_entries.size()  ,
            0xFF
        );

        memcpy(rt.data() + 0, &length, sizeof(length));
        memcpy(rt.data() + sizeof(length), &slot_card_in_use, sizeof(slot_card_in_use));

        std::size_t offset = sizeof(length) + sizeof(slot_card_in_use);
        for (config_signature_t sig : device_entries)
        {
            memcpy(rt.data() + offset, &sig, sizeof(sig));
            offset += sizeof(config_signature_t);
        }

        return rt;
    }
};

struct one_wire
{
    uint8_t version;
    // uint8_t checksum;
    device_signature_t signature;
    char part_number[15];
    // uint16_t len_device_entries;
    // uint16_t len_config_entries;
    std::vector<ow_device_entry> device_entries;
    std::vector<ow_config_entry> config_entries;

    std::vector<uint8_t> serialize() const
    {
        // Serialize and union device entries.
        std::vector<uint8_t> devices;
        std::vector<uint8_t> configs;

        {
            std::vector<std::vector<uint8_t>> seg_devices;
            std::size_t size = 0;
            for(auto var : device_entries)
            {
                auto vec = var.serialize();
                size += vec.size();
                seg_devices.push_back(vec);
            }

            devices.resize(size);
            std::size_t offset = 0;
            for (auto vec : seg_devices)
            {
                memcpy(devices.data() + offset, vec.data(), vec.size());
                offset += vec.size();
            }
        }
        {
            constexpr std::size_t UNIT_SIZE = sizeof(ow_config_entry::sig) + sizeof(uint16_t);
            std::vector<uint8_t> config_headers
            (
                UNIT_SIZE*config_entries.size(),
                0xFF
            );

            std::size_t payloads_size = 0;
            for (std::size_t i = 0; i < config_entries.size(); ++i)
            {
                const ow_config_entry& r_entry = config_entries[i];
                const uint16_t INDEX = config_headers.size() + payloads_size;
                const std::size_t OFFSET = UNIT_SIZE*i;
                memcpy(config_headers.data() + OFFSET, &r_entry.sig, sizeof(r_entry.sig));
                memcpy(config_headers.data() + OFFSET + sizeof(r_entry.sig), &INDEX, sizeof(INDEX));

                payloads_size += r_entry.payload.size();
            }

            configs.resize(config_headers.size() + payloads_size);
            memcpy(configs.data(), config_headers.data(), config_headers.size());

            std::size_t offset = config_headers.size();
            for (auto var : config_entries)
            {
                memcpy(configs.data() + offset, var.payload.data(), var.payload.size());
                offset += var.payload.size();
            }


        }

        uint16_t devices_len = devices.size();
        uint16_t configs_len = configs.size();

        std::vector<uint8_t> rt(
            sizeof(version) + sizeof(uint8_t) + sizeof(signature) + sizeof(part_number) +
            2*sizeof(uint16_t) + devices.size() + configs.size(), 0xFF
        );

        memcpy(rt.data() + 0,
            &version, sizeof(version));
        memcpy(rt.data() + sizeof(version) + sizeof(uint8_t),
            &signature, sizeof(signature));
        memcpy(rt.data() + sizeof(version) + sizeof(uint8_t) + sizeof(signature),
            part_number, sizeof(part_number));
        memcpy(rt.data() + sizeof(version) + sizeof(uint8_t) + sizeof(signature) + sizeof(part_number),
            &devices_len, sizeof(devices_len));
        memcpy(rt.data() + sizeof(version) + sizeof(uint8_t) + sizeof(signature) + sizeof(part_number) + sizeof(devices_len),
            &configs_len, sizeof(configs_len));
        memcpy(rt.data() + sizeof(version) + sizeof(uint8_t) + sizeof(signature) + sizeof(part_number) + sizeof(devices_len) + sizeof(configs_len),
            devices.data(), devices.size());
        memcpy(rt.data() + sizeof(version) + sizeof(uint8_t) + sizeof(signature) + sizeof(part_number) + sizeof(devices_len) + sizeof(configs_len) + devices.size(),
            configs.data(), configs.size());

        // Calculate checksum
        rt[1] = rt[0];
        for (std::size_t i = 2; i < rt.size(); ++i)
        {
            rt[1] += rt[i];
        }

        return rt;
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
int main (int argc, char** argv)
{
    one_wire rt;
    rt.version = static_cast<uint8_t>(DEFAULT_VERSION);

    ow_args args;

    try
    {
        args = parse_ow_args(argc, argv);
    }
    catch (std::exception& r_e)
    {
        std::cerr << "Argument Error:  " << r_e.what() << "\n";
        return 1;
    }

    if (args.need_help)
    {
        print_ow_usage(argv[0]);
        return 0;
    }

    // Read in file
    tinyxml2::XMLDocument doc;
    {
        auto error = doc.LoadFile(args.input_file.string().c_str());
        if (error != tinyxml2::XML_SUCCESS)
        {
            std::cerr << "Failed to open XML file.  Error code: " << std::to_string(error) << "\n";
            return 1;
        }
        preprocess_xml(doc, args.input_file);
    }

    // Get signature, part number, device signatures, and config signatures.
    auto p_ow = doc.RootElement();
    if (p_ow->Name() != std::string("OneWire"))
    {
        std::cerr << "Root node is not \"OneWire\"\n";
        return 1;
    }

    auto p_sig = p_ow->FirstChildElement("Signature");
    if (p_sig == nullptr)
    {
        std::cerr << "Failed to find XML element \"Signature\" in \"OneWire\" node.\n";
        return 1;
    }
    auto p_sig_dst = p_sig->FirstChildElement("DefaultSlotType");
    if (p_sig_dst == nullptr)
    {
        std::cerr << "Failed to find XML element \"DefaultSlotType\" in \"Signature\" node.\n";
        return 1;
    }
    auto p_sig_did = p_sig->FirstChildElement("DeviceID");
    if (p_sig_did == nullptr)
    {
        std::cerr << "Failed to find XML element \"DeviceID\" in \"Signature\" node.\n";
        return 1;
    }
    rt.signature =
    {
        static_cast<slot_types>(std::stoul(p_sig_dst->Attribute("value"))),
        static_cast<device_id_t>(std::stoul(p_sig_did->Attribute("value")))
    };

    auto p_part_number = p_ow->FirstChildElement("PartNumber");
    if (p_part_number == nullptr)
    {
        std::cerr << "Failed to find XML element \"PartNumber\" in \"OneWire\" node.\n";
        return 1;
    }
    std::string part_number = p_part_number->Attribute("value");
    if (part_number.size() > 15) // DEVICE_DETECT_PART_NUMBER_LENGTH
    {
        std::cerr << "Part number is too long (max 15).\n";
        return 1;
    }
    memset(rt.part_number, '\0', sizeof(rt.part_number));
    std::strncpy(rt.part_number, part_number.c_str(), sizeof(rt.part_number) - 1);

    auto p_device_configs = p_ow->FirstChildElement("DeviceConfigurations");
    if (p_device_configs == nullptr)
    {
        std::cerr << "Failed to find XML element \"DeviceConfigurations\" in \"OneWire\" node.\n";
        return 1;
    }
    auto p_config_entries = p_ow->FirstChildElement("ConfigEntries");
    if (p_config_entries == nullptr)
    {
        std::cerr << "Failed to find XML element \"ConfigEntries\" in \"OneWire\" node.\n";
        return 1;
    }

    // Compile device signatures
    tinyxml2::XMLElement * p_device_entry = p_device_configs->FirstChildElement();
    while (p_device_entry != nullptr)
    {
        compiled_entry_slim slim;

        try
        {
            compile_device(p_device_entry, slim);
        }
        catch(const std::exception& e)
        {
            std::cerr << "Device Compilation Error:  " << e.what() << '\n';
            return 1;
        }
        
        rt.device_entries.emplace_back(slim);

        p_device_entry = p_device_entry->NextSiblingElement();
    }

    // Compile config signatures
    tinyxml2::XMLElement * p_config_entry = p_config_entries->FirstChildElement();
    while (p_config_entry != nullptr)
    {
        compiled_entry_slim slim;

        try
        {
            compile_config(p_config_entry, slim);
        }
        catch(const std::exception& e)
        {
            std::cerr << "Config Compilation Error:  " << e.what() << '\n';
            return 1;
        }

        rt.config_entries.emplace_back(slim);

        p_config_entry = p_config_entry->NextSiblingElement();
    }

    // Emplace into contiguous buffer.
    auto serialized = rt.serialize();

    // Check buffer size
    if (args.maximum_ow_size > 0 && serialized.size() > args.maximum_ow_size)
    {
        std::cerr << "Sizing error\nMaximum\t" << args.maximum_ow_size << "\nActual\t" << serialized.size() << '\n';
        return 1;
    }

    // Return
    std::fstream out_file;
    out_file.open(args.output_file, std::ios::binary | std::ios::out);
    if (!out_file.is_open())
    {
        throw std::logic_error(std::string("failed to open file"));
    }
    out_file.write(reinterpret_cast<char*>(serialized.data()), static_cast<std::streamsize>(serialized.size()));
    out_file.close();

    return 0;
}


/*****************************************************************************
 * Private Functions
 *****************************************************************************/

// EOF
