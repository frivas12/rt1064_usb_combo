OUTPUT_DIR := "bin/"
FIRMWARE_DIR = "../Firmware/"

packages := $(basename $(notdir $(wildcard packaging/*.mk)))
unsealed_pacakges := $(addsuffix .unsealed, $(packages))

.PHONY : all clean list $(packages) $(unsealed_pacakges)

all: $(packages)

clean:
	@rm -rf bin/ build/

list:
	@echo $(packages)

$(packages) : % : packaging/%.mk
	@$(MAKE) --no-print-directory -f tools/packager.mk PACKAGE_CONFIG=$< package

$(unsealed_pacakges) : %.unsealed : packaging/%.mk
	@$(MAKE) --no-print-directory -f tools/packager.mk PACKAGE_CONFIG=$< package.unsealed
