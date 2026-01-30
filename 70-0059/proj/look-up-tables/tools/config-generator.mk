# This makefile uses the update-analysis to rebuild to CONFIG_SRCS
# in the second makefile.
#
#
include tools/config.mk

ifndef OUTPUT_PATH
$(error Output variable "OUTPUT_PATH" must be assigned.)
endif

ifndef TOML
$(error TOML input file variable "TOML" must be assigned.)
endif

ifndef CONFIG
$(error Config variable "CONFIG" must be assigned.)
endif
# Required variables from config file:
# DEVICE_SRCS:  A space-separated list of device sources (under device/)

include $(CONFIG)
device_obj = $(DEVICE_SRCS)

$(OUTPUT_PATH): $(device_obj)
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@"$(CONFIG2_GENERATOR)" < "$(TOML)" > "$@" || rm -f "$@"

.PHONY: clean
clean:
	@rm -rf $(OUTPUT_PATH)
