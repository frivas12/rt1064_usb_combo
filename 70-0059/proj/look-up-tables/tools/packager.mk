include tools/config.mk

ifndef CONFIG
$(error Config variable "CONFIG" must be assigned.)
endif
# Required variables from config file:
# PACKAGE_NAME:	The name of the package
# VERSION:		A semantic version
# CONFIG_SRCS:  A space-separated list of config sources (under config/)
# DEVICE_SRCS:  A space-separated list of device sources (under device/)

include $(CONFIG)
config_obj = $(addprefix $(INTERMEDIATE_DIR)/, $(addsuffix .lo, $(basename $(CONFIG_SRCS))))
device_obj = $(addprefix $(INTERMEDIATE_DIR)/, $(addsuffix .lo, $(basename $(DEVICE_SRCS))))

$(OUTPUT_DIR)/$(PACKAGE_NAME).zip: $(INTERMEDIATE_DIR)/pack_$(PACKAGE_NAME)/config-lut.bin \
								   $(INTERMEDIATE_DIR)/pack_$(PACKAGE_NAME)/device-lut.bin \
								   $(INTERMEDIATE_DIR)/pack_$(PACKAGE_NAME)/lut-version.bin
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "package - lut\t$@"
	@zip -jq0 -o "$@" $^

.PHONY: clean
clean:
	@rm -rf $(OUTPUT_DIR)/$(PACKAGE_NAME).$(VERSION).zip $(INTERMEDIATE_DIR)/pack_$(PACKAGE_NAME)/

$(INTERMEDIATE_DIR)/pack_$(PACKAGE_NAME)/config-lut.bin: $(config_obj)
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "lutc\t\t$@"
	@$(LUTC) -w 2,2 -s auto $(config_obj) -o $@

$(INTERMEDIATE_DIR)/pack_$(PACKAGE_NAME)/device-lut.bin: $(device_obj)
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "lutc\t\t$@"
	@$(LUTC) -w 2,2,2 -s auto $(device_obj) -o $@

$(INTERMEDIATE_DIR)/pack_$(PACKAGE_NAME)/lut-files.csv: $(INTERMEDIATE_DIR)/pack_$(PACKAGE_NAME)/config-lut.bin \
													 			   $(INTERMEDIATE_DIR)/pack_$(PACKAGE_NAME)/device-lut.bin
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "0,$(INTERMEDIATE_DIR)/pack_$(PACKAGE_NAME)/config-lut.bin" > $@
	@echo -e "1,$(INTERMEDIATE_DIR)/pack_$(PACKAGE_NAME)/device-lut.bin" >> $@
	
$(INTERMEDIATE_DIR)/pack_%/lut-version.bin: $(INTERMEDIATE_DIR)/pack_$(PACKAGE_NAME)/lut-files.csv
													   
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "version\t\t$@"
	@"$(VERSION_COMPILER)" -o $@ $< $(VERSION)

$(INTERMEDIATE_DIR)/config/%.lo : config/%.xml
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "lutc - config\t$@"
	@$(LUTC) -w 2,2 -c $< -o $@

$(INTERMEDIATE_DIR)/device/%.lo : device/%.xml
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "lutc - device\t$@"
	@$(LUTC) -w 2,2,2 -c $< -o $@
