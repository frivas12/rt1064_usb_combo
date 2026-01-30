LUT_COMPILER_PROJECT_DIR := ../LUT_Builder
LUTC := $(LUT_COMPILER_PROJECT_DIR)/bin/lutc.exe
OWC := $(LUT_COMPILER_PROJECT_DIR)/bin/owc.exe
PACKAGE_MAKEFILE_GENERATOR := "python3 package_requirement_analyzer.py"
CONFIG1_GENERATOR := "python3 tools/config1_builder.py"
CONFIG2_GENERATOR := "python3 tools/config2_builder.py"
VERSION_COMPILER := "python3 tools/version_compiler.py"
SQLITE3 := "sqlite3"

OUTPUT_DIR := bin
INTERMEDIATE_DIR := build
