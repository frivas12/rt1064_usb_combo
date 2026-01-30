CXX:=g++
CXXFLAGS=-fdiagnostics-color=always -g -O0 -Wall -Wsign-conversion -std=c++17 -D_DESKTOP_BUILD -D _TESTING
TEMPLATE_MAKEFILE=TemplateMakefile

INCLUDE_DIRS := \
-Isrc/ \
-I../Firmware/src/ \
-I../Firmware/src/system/slots/ \
-I../Firmware/src/system/drivers/device_detect/ \
-I../Firmware/src/system/drivers/lut/ \
-I../Firmware/src/system/drivers/save_constructor/ \
-I../Firmware/src/system/drivers/cpp_allocator/ \
-I../Firmware/src/system/drivers/encoder/ \
-I../Firmware/src/system/drivers/limits/ \
-I../Firmware/src/system/drivers/efs/ \
-I../Firmware/src/system/drivers/eeprom/mapper/ \
-I../Firmware/src/system/cards/OTM/ \
-I../Firmware/src/system/cards/piezo/ \
-I../Firmware/src/system/cards/piezo_elliptec/ \
-I../Firmware/src/system/cards/servo/ \
-I../Firmware/src/system/cards/shutter/ \
-I../Firmware/src/system/cards/slider_io/ \
-I../Firmware/src/system/cards/stepper/ \
-I../Firmware/src/system/sync/gate/ \
-I../Firmware/src/system/sync/lock_guard/ \
-Ithirdparty/Catch2/build/generated-includes/ \
-Ithirdparty/Catch2/extras



SRC_PATHS=\
src/*.cc \
src/tests/*.cc \
thirdparty/Catch2/extras/catch_amalgamated.cpp

# Source paths *not* descended from this Makefile's directory.
REMOTE_SRC_PATHS=\
../Firmware/src/system/drivers/efs/*.cc \
../Firmware/src/system/drivers/lut/*.cc \
../Firmware/src/system/drivers/save_constructor/save_constructor.cc \
../Firmware/src/system/drivers/save_constructor/save_size.cc \
../Firmware/src/system/sync/lock_guard/*.cc \
../Firmware/src/defs.cc \


TEMPLATE_PATHS=\
../Firmware/src/system/drivers/lut/*.tcc

ifdef USE_COVERAGE
CXXFLAGS+=-fprofile-arcs -ftest-coverage
endif


all_src := $(wildcard $(SRC_PATHS))
all_remote_src := $(wildcard $(REMOTE_SRC_PATHS))

output_dir := build
remote_output_dir := build/remote

objs := $(addprefix $(output_dir)/, $(addsuffix .o, $(basename $(all_src))))
remote_objs := $(addprefix $(remote_output_dir)/, $(patsubst ../%,%, $(addsuffix .o, $(basename $(all_remote_src)))))
# coverage_files := $(addsuffix .gcno, $(basename $(all_src)))
# testing_files := $(addsuffix .gcda, $(basename $(all_src)))
# also_coverage_files := $(addsuffix .gcov, $(basename $(all_src)))
templates := $(wildcard $(TEMPLATE_PATHS))

output := bin/tests.exe
report_dir := build/reports/

all: $(TARGET_MAKEFILE) target

coverage: build
	$(MAKE) USE_COVERAGE=1
	$(output)
#	./move_coverage_files.sh $(coverage_files) $(testing_files) $(also_coverage_files)
	./move_coverage_files.sh

build:
	mkdir build

$(TEMPLATE_MAKEFILE):
	python create_template_dependencies.py $(all_src) $(all_remote_src) $(templates) $(INCLUDE_DIRS) > $(TEMPLATE_MAKEFILE)

target: $(output)

report: coverage
	gcov $(objs)

$(output): $(objs) $(remote_objs)
	$(CXX) $(CXXFLAGS) -o $(output) $(objs) $(remote_objs)

include TemplateMakefile

$(output_dir)/%.o: %.cc
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "$(CXX)\t\t$@"
	@$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

$(output_dir)/%.o: %.cpp
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "$(CXX)\t\t$@"
	@$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

$(remote_output_dir)/%.o: ../%.cc
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "$(CXX)(remote)\t$@"
	@$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

$(remote_output_dir)/%.o: ../%.cpp
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "$(CXX)(remote)\t$@"
	@$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

.PHONY : clean
clean:
	rm -f $(objs) $(remote_objs) $(output) $(TEMPLATE_MAKEFILE)
	rm -rf $(output_dir)