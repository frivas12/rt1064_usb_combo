TARGETS := MCM301 MCM6101 MCM6201
CHECK_TARGETS := $(addsuffix .check, $(TARGETS))
CLEAN_TARGETS := $(addsuffix .clean, $(TARGETS))

.PHONY: all clean $(TARGETS) $(CLEAN_TARGETS) $(CHECK_TARGETS)

all: $(TARGETS)

$(TARGETS): % : %.check
	@$(MAKE) --no-print-directory -f builder.mk TARGET=$*

$(CHECK_TARGETS): %.check :
	@test -d build/targets/$*/include || mkdir -p build/targets/$*/include
	@$(MAKE) -s -f builder.mk TARGET=$* -q || cat ./src/version-build.h.template | sed "s/{{time}}/`date +%s`/" > ./build/targets/$*/include/version-build.h

clean:
	@rm -rf bin/ build/

$(CLEAN_TARGETS): %.clean :
	@rm -rf bin/$* build/targets/$*
