include $(PACKAGE_CONFIG)
# FGPN, PRODUCT_ID, SCHEMA
# FIRMWARE_FILE, FIRMWARE_VERSION
# CPLD_FILE, CPLD_VERSION
# LUT_FILE, LUT_VERSION

PACKAGE_VERSION := $(shell tools/get-package-version.sh $(FIRMWARE_VERSION) $(CPLD_VERSION) $(LUT_VERSION))

OUTPUT := bin/$(FIRMWARE_TARGET).$(PACKAGE_VERSION).mcmpkg

.DELETE_ON_ERROR: $(OUTPUT)
.PHONY: package package.unsealed

package: $(OUTPUT)

package.unsealed: build/$(FGPN)/unsealed.$(FIRMWARE_TARGET).$(PACKAGE_VERSION).mcmpkg

$(OUTPUT): build/$(FGPN)/unsealed.$(FIRMWARE_TARGET).$(PACKAGE_VERSION).mcmpkg
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "mcmpack (6/7)\t(check-health)"
	@mcmpack --package="$<" checkhealth $(MCMPACK_WFLAGS)
	@echo -e "mcmpack (7/7)\t> $@"
	@mcmpack --package="$<" seal -f -o "$@" bootloader:1313:2F03 application:1313:$(PRODUCT_ID) >/dev/null

build/$(FGPN)/unsealed.$(FIRMWARE_TARGET).$(PACKAGE_VERSION).mcmpkg: build/$(FGPN)/schema.xml \
																	 $(FIRMWARE_FILE) \
		   															 $(CPLD_FILE) \
		   															 $(LUT_FILE)
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@rm -f "$@"
	@echo -e "mcmpack (1/7)\t> $@"
	@mcmpack create $(FGPN) $(PACKAGE_VERSION) -o "$@" >/dev/null
	@echo -e "mcmpack (2/7)\t< build/$(FGPN)/schema.xml"
	@mcmpack --package="$@" schema import build/$(FGPN)/schema.xml
	@echo -e "mcmpack (3/7)\t< $(FIRMWARE_FILE)"
	@mcmpack --package="$@" add-file "$(FIRMWARE_FILE)" --key=firmware.hex --version=$(FIRMWARE_VERSION)
	@mcmpack --package="$@" programmable link firmware firmware.hex
	@echo -e "mcmpack (4/7)\t< $(CPLD_FILE)"
	@mcmpack --package="$@" add-file "$(CPLD_FILE)" --key=cpld.jed --version=$(CPLD_VERSION)
	@mcmpack --package="$@" programmable link cpld cpld.jed
	@echo -e "mcmpack (5/7)\t< $(LUT_FILE)"
	@mcmpack --package="$@" add-file "$(LUT_FILE)" -x --key=lut --version=$(LUT_VERSION)
	@mcmpack --package="$@" programmable link lut.config lut/config-lut.bin
	@mcmpack --package="$@" programmable link lut.device lut/device-lut.bin
	@mcmpack --package="$@" programmable link lut.version lut/lut-version.bin
	@mcmpack --package="$@" programmable link lut lut/lut-version.bin

build/$(FGPN)/schema.xml: schema/$(SCHEMA).xml
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@tools/schema-generator.sh $< $(FGPN) > $@
