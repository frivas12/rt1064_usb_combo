include tools/config.mk

onewire_rel_objs = $(addsuffix .bin, $(basename $(shell cd onewire/ && find . -name "*.xml" -printf '%P\n')))
onewire_objs = $(addprefix $(INTERMEDIATE_DIR)/onewire/, $(onewire_rel_objs))

package_names = $(basename $(notdir $(wildcard packaging/*.toml)))
package_objs = $(addprefix $(OUTPUT_DIR)/, $(addsuffix .zip, $(package_names)))
database_sqls = $(wildcard database/*.sql)

.PHONY : all clean db dbexport luts onewire $(package_names)

all: db luts onewire

db: $(INTERMEDIATE_DIR)/global.db

luts: $(package_names)

onewire: $(OUTPUT_DIR)/onewire.zip

$(package_names): % : $(OUTPUT_DIR)/%.zip

dbexport: $(INTERMEDIATE_DIR)/global.db
	@$(SQLITE3) $< < tools/exportdb.sql
	@echo -e "exported $< to database/"

$(INTERMEDIATE_DIR)/global.db: $(database_sqls)
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@rm -f "$@"
	@echo -e "database\t$@"
	@$(SQLITE3) $@ < tools/importdb.sql

$(OUTPUT_DIR)/%.zip: $(INTERMEDIATE_DIR)/pack_%/config.mk FORCE
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@$(MAKE) --no-print-directory -f tools/packager.mk CONFIG=$<

$(OUTPUT_DIR)/onewire.zip: $(onewire_objs)
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "package - ow\t$@"
	@cd $(INTERMEDIATE_DIR)/onewire/ && \
		gzip -q0 -o "$(addprefix ../../, $@)" $(onewire_rel_objs)


# PHONY is not valid.
FORCE:


$(INTERMEDIATE_DIR)/pack_%/config.mk: $(INTERMEDIATE_DIR)/pack_%/config1.mk \
									  $(INTERMEDIATE_DIR)/pack_%/config2.mk
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@cat "$(INTERMEDIATE_DIR)/pack_$*/config1.mk" > "$@"
	@cat "$(INTERMEDIATE_DIR)/pack_$*/config2.mk" >> "$@"

$(INTERMEDIATE_DIR)/pack_%/config1.mk: packaging/%.toml $(INTERMEDIATE_DIR)/global.db
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "analyzing 1/2\t$<"
	@"$(CONFIG1_GENERATOR)" < "$<" > "$@" || rm -f $@

# Device analysis should remove device LUT.
	@rm -f "$(INTERMEDIATE_DIR)/pack_$*/device-lut.bin"

$(INTERMEDIATE_DIR)/pack_%/config2.mk: packaging/%.toml \
									   $(INTERMEDIATE_DIR)/pack_%/config1.mk \
									   $(INTERMEDIATE_DIR)/global.db
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "analyzing 2/2\t$<"
	@$(MAKE) -s -f tools/config-generator.mk CONFIG="$(INTERMEDIATE_DIR)/pack_$*/config1.mk" OUTPUT_PATH="$@" TOML="$<"

# Config analysis should remove config LUT.
	@rm -f "$(INTERMEDIATE_DIR)/pack_$*/config-lut.bin"

# (sbenish) GNU Make considers confg1.mk, config2.mk, and config.mk to be
# temporary files that will be cleaned up after analysis is performed.
# Since that would incur a large penalty, the joined makefile is kept
# to prevent analysis on every build until the TOML file changes.
.PRECIOUS: $(INTERMEDIATE_DIR)/pack_%/config.mk

$(INTERMEDIATE_DIR)/onewire/%.bin: onewire/%.xml
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "owc\t\t$@"
	@"$(OWC)" -s 128 "$<" -o "$@"


clean:
	rm -rf $(OUTPUT_DIR) $(INTERMEDIATE_DIR)
