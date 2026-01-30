FGPN := MCM301
PRODUCT_ID := 2016
LUT_PACKAGE := mcm301
SCHEMA := v1

MCMPACK_WFLAGS := --Werror --Wversion --Wfile --Wempty-lut --Wnostatepref --Wconnected-graph --Wdefault-cmd-length
CPLD_FILE := ../cpld/impl_mcm/mcm_top_impl_mcm.jed
CPLD_VERSION := $(shell tools/get-cpld-version.sh)

# Derivative
FIRMWARE_TARGET := $(FGPN)

FIRMWARE_FILE := ../Firmware/bin/$(FIRMWARE_TARGET)/$(FIRMWARE_TARGET).hex
LUT_FILE := ../look-up-tables/bin/$(LUT_PACKAGE).zip

FIRMWARE_VERSION := $(shell tools/get-fw-version.sh)
LUT_VERSION := $(shell tools/get-lut-version.sh ../look-up-tables/packaging/$(LUT_PACKAGE).toml)
