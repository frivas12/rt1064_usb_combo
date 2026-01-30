#pragma once

#include <filesystem>

#include "tinyxml2.h"

#include "lut-builder/intermediate-formats.hh"


void compile_config(tinyxml2::XMLElement * p_parent, compiled_entry_slim& r_slim);
void compile_device(tinyxml2::XMLElement * p_parent, compiled_entry_slim& r_slim);

void compile(compiled_entry_slim& r_slim, const std::filesystem::path& r_path);
