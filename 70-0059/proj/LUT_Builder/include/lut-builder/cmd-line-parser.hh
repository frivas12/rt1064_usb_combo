// command-line-parser.hh

/**************************************************************************//**
 * \file command-line-parser.hh
 * \author Sean Benish
 * \brief Tranlates command-line arguments into programmatic inputs.
 *****************************************************************************/
#pragma once

#include <string>
#include <filesystem>
#include <vector>
#include <cstdint>
#include <optional>

/*****************************************************************************
 * Defines
 *****************************************************************************/

enum class target_types_e
{
    LUT,
    LUT_OBJECT
};

enum class one_wire_versions_e : uint8_t
{
    VERSION_1 = 250,
    VERSION_2 = 251,
};

constexpr char LUT_OBJECT_EXTENSION[] = ".lo";

/*****************************************************************************
 * Data Types
 *****************************************************************************/
struct compilation_target
{
    std::vector<std::filesystem::path> input_file_paths;
    std::filesystem::path output_file_path;
    target_types_e target_type;
};

struct structured_arguments
{
    bool need_help;
    std::vector<compilation_target> targets;
    bool optimize_page_size;
    std::size_t lut_page_size;
    std::vector<std::size_t> key_sizes;
};

struct ow_args
{
    bool need_help;
    std::filesystem::path input_file;
    std::filesystem::path output_file;  // TODO(sbenish):  Change this into an OStream to allow for std::cout porting.
    std::size_t maximum_ow_size;        ///> Maximum size (in bytes) of the one-wire EEPROM.
};



/*****************************************************************************
 * Constants
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
structured_arguments parse_args(int argc, char ** ps_argv);
void print_usage(const char * const s_arg0);

ow_args parse_ow_args(int argc, char** ps_argv);
void print_ow_usage(const char * const s_arg0);

//EOF
