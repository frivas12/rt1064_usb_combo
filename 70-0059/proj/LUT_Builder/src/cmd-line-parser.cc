// cmd-line-parser.cc

#include "lut-builder/cmd-line-parser.hh"

#include <getopt.h>
#include <iostream>
#include <limits>
#include <regex>
#include <stdexcept>

/*****************************************************************************
 * Constants
 *****************************************************************************/
constexpr char LUTC_SHORT_OPTIONS[] = "w:s:o:c?";
static const struct option LUTC_LONG_OPTIONS[] = {
    /*NAME          ARGUMENT            FLAG        SHORT NAME*/
    {"width", required_argument, NULL, 'w'},
    {"size", required_argument, NULL, 's'},
    {"output", required_argument, NULL, 'o'},
    {"help", no_argument, NULL, '?'},
    {NULL, 0, NULL, 0}};

constexpr char OWC_SHORT_OPTIONS[] = "s:o:?";
static const struct option OWC_LONG_OPTIONS[] = {
    {"size", required_argument, NULL, 's'},
    {"output", required_argument, NULL, 'o'},
    {"help", no_argument, NULL, '?'},
    {NULL, 0, NULL, 0}};

static const std::regex INTEGER_REGEX("\\d+");

/*****************************************************************************
 * Macros
 *****************************************************************************/

/*****************************************************************************
 * Data Types
 *****************************************************************************/

/*****************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
static std::size_t get_int(const std::string &str_int);

/*****************************************************************************
 * Static Data
 *****************************************************************************/

/******************************************************************************
 * Interrupt Handlers
 *****************************************************************************/

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
structured_arguments parse_args(int argc, char **ps_argv) {
  structured_arguments rt;
  rt.optimize_page_size = false;
  rt.need_help = false;

  std::string output_val = "";
  bool compiling_to_intermediate = false;

  int c;
  int opt_index;
  while ((c = getopt_long(argc, ps_argv, LUTC_SHORT_OPTIONS, LUTC_LONG_OPTIONS,
                          &opt_index)) != -1) {
    switch (c) {
    case 0:
      // No short-equivalent long options
      throw std::invalid_argument(std::string("Could not parse argument \"") +
                                  LUTC_LONG_OPTIONS[opt_index].name + "\"");
      break;
    case 'c':
      compiling_to_intermediate = true;
      break;
    case 'o':
      output_val = std::string(optarg);
      break;
    case 's': {
      std::string val = optarg;
      if (val == "auto") {
        // (sbenish) Setting the page size of max
        // and the optimizer will slim it down.
        rt.lut_page_size = std::numeric_limits<std::size_t>::max();
        rt.optimize_page_size = true;
      } else if (std::regex_match(val, INTEGER_REGEX)) {
        rt.lut_page_size = std::stoull(val);
        rt.optimize_page_size = false;
      } else {
        throw std::invalid_argument(std::string(
            "Size argument should be \"auto\" or an integer, was " + val));
      }
    } break;
    case 'w': {
      std::string val = optarg;
      std::string::size_type found_index;
      std::string::size_type start_index = 0;
      while ((found_index = val.find(',', start_index)) != std::string::npos) {
        rt.key_sizes.push_back(
            get_int(val.substr(start_index, found_index - start_index)));
        start_index = found_index + 1;
      }
      rt.key_sizes.push_back(get_int(val.substr(start_index)));
    } break;
    case '?':
      rt.need_help = true;
      break;
    default:
      throw std::invalid_argument(std::string("Could not parse character '") +
                                  std::to_string(c) + "'");
      break;
    }
  }

  // Non option arguments
  if (!compiling_to_intermediate) {
    rt.targets.push_back(compilation_target{});
  }
  if (optind >= argc && !rt.need_help) {
    throw std::invalid_argument("Did not get any input files.");
  }
  while (optind < argc) {
    if (compiling_to_intermediate) {
      compilation_target target;
      target.input_file_paths.push_back(
          std::filesystem::path(ps_argv[optind++]));
      rt.targets.push_back(target);
    } else {
      rt.targets[0].input_file_paths.push_back(
          std::filesystem::path(ps_argv[optind++]));
    }
  }

  if (compiling_to_intermediate && output_val != "" && rt.targets.size() > 1) {
    throw std::invalid_argument(std::string(
        "Cannot set an intermediate output with multiple input files."));
  }

  if (compiling_to_intermediate) {
    // Setting target output paths for intermediate compliation.
    for (auto itr = rt.targets.begin(); itr != rt.targets.end(); ++itr) {
      itr->target_type = target_types_e::LUT_OBJECT;
      if (output_val == "") {
        itr->output_file_path = std::filesystem::path(itr->input_file_paths[0])
                                    .replace_extension(LUT_OBJECT_EXTENSION);
      } else {
        itr->output_file_path = std::filesystem::path(output_val);
      }
    }
  } else {
    // Setting target output path for LUT compliation.
    rt.targets[0].target_type = target_types_e::LUT;
    if (output_val == "") {
      rt.targets[0].output_file_path =
          std::filesystem::current_path() / "lut.bin";
    } else {
      rt.targets[0].output_file_path = std::filesystem::path(output_val);
    }
  }

  // Change default keys sizes if not parsed
  if (rt.key_sizes.size() == 0) {
    rt.key_sizes.push_back(4);
    rt.key_sizes.push_back(4);
  }

  return rt;
}

void print_usage(const char *const s_arg0) {
  std::cout << "Usage: " << s_arg0 << " [opts] <supported file>...\n"
            << "Arguments:\n"
            << "\tAny number of XML files to compile or "
            << LUT_OBJECT_EXTENSION << " files.\n"
            << "Options:\n"
            << "\t-w <cs-int>, --width=<cs-int>\n"
            << "\t\tComma-serparted list for the number of bytes that each key "
               "takes up.\n"
            << "\t\tEg) --width=1,2,4 defines a LUT with 1 level of "
               "indirection, an indirection key of size 1, a structure key of "
               "size 2, and a discriminator key of size 4.\n"
            << "\t-s <int>, --size=<int>\n"
            << "\t\tSets the size of the LUT pagesin bytes.\n"
            << "\t-s auto, --size=auto\n"
            << "\t\tThe LUT page size will be optimized to the smallest size.\n"
            << "\t-o <path>, --output=<path>\n"
            << "\t\tSets the output path for the output object.\n"
            << "\t-c\n"
            << "\t\tCompiles the XMLs into an intermediate format ("
            << LUT_OBJECT_EXTENSION << ").\n";
}

ow_args parse_ow_args(int argc, char **ps_argv) {
  ow_args rt;
  rt.maximum_ow_size = 0;
  rt.need_help = false;

  std::string out_path = "";

  int c;
  int opt_index;
  while ((c = getopt_long(argc, ps_argv, OWC_SHORT_OPTIONS, OWC_LONG_OPTIONS,
                          &opt_index)) != -1) {
    switch (c) {
    case 0:
      // No short-equivalent long options
      throw std::invalid_argument(std::string("Could not parse argument \"") +
                                  OWC_LONG_OPTIONS[opt_index].name + "\"");
      break;
    case 'o':
      out_path = std::string(optarg);
      break;
    case 's': {
      std::string val = optarg;
      if (std::regex_match(val, INTEGER_REGEX)) {
        rt.maximum_ow_size = std::stoull(val);
      } else {
        throw std::invalid_argument(
            std::string("Size argument should be an integer, was " + val));
      }
    } break;
    case '?':
      rt.need_help = true;
      break;
    default:
      throw std::invalid_argument(std::string("Could not parse character '") +
                                  std::to_string(c) + "'");
      break;
    }
  }

  // Non option arguments
  if (!rt.need_help) {
    if (optind >= argc) {
      throw std::invalid_argument("Did not get any input files.");
    }
    if (optind + 1 != argc) {
      throw std::invalid_argument("Too many arguments");
    }
    rt.input_file = ps_argv[optind];
  }

  if (out_path == "") {
    rt.output_file = std::filesystem::current_path() / "one-wire.bin";
  } else {
    rt.output_file = out_path;
  }

  return rt;
}

void print_ow_usage(const char *const s_arg0) {
  std::cout << "Usage:  " << s_arg0 << " [opts] <one-wire xml>\n"
            << "Argument:\n"
            << "\tOne One-Wire XML file to compile.\n"
            << "Options:\n"
            << "\t-s <int>, --size=<int>\n"
            << "\t\tSets the maximum size of the one-wire EEPROM to generate "
               "size overflow limits.\n"
            << "\t\tIf not set, size overflow limits are not generated.\n"
            << "\t-o <path>, --output=<path>\n"
            << "\t\tSets the output path for the compiled objected.\n";
}

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
static std::size_t get_int(const std::string &str_int) {
  if (std::regex_match(str_int, INTEGER_REGEX)) {
    return std::stoull(str_int);
  } else {
    throw std::invalid_argument(
        std::string("Width argument should be an integer, not" + str_int));
  }
}
// EOF
