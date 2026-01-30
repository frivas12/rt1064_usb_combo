C=gcc
CFLAGS=-fdiagnostics-color=always -Wall -D_DESKTOP_BUILD -fno-omit-frame-pointer -static-libgcc --std=c11
CXX=g++
CXXFLAGS=-fdiagnostics-color=always -Wall -Wsign-conversion -D_DESKTOP_BUILD -fno-omit-frame-pointer -static-libstdc++ --std=c++20

ifdef RELEASE
CFLAGS+=-O2
CXXFLAGS+=-O2
else
CFLAGS+=-g -O0
CXXFLAGS+=-g -O0
endif

OUTPUT_DIR := build
OUTPUT_EXE := lut-build.exe

INCLUDE_DIR := \
-I../Firmware/src/system/cards/stepper/ \
-I../Firmware/src/system/cards/servo/ \
-I../Firmware/src/system/cards/slider_io/ \
-I../Firmware/src/system/cards/shutter/ \
-I../Firmware/src/system/cards/piezo/ \
-I../Firmware/src/system/drivers/limits/ \
-I../Firmware/src/system/drivers/eeprom/mapper/ \
-I../Firmware/src/system/drivers/encoder/ \
-I../Firmware/src/system/drivers/lut/ \
-I../Firmware/src/ \
-I../Firmware/src/system/slots/ \
-I../Firmware/src/system/drivers/save_constructor/ \
-I../Firmware/src/system/driver/cpp_allocator \
-Iinclude/ \
-Ilibs/tinyxml2-11.0.0 \

SRC_PATHS := \
src/*.cc \
src/workers/*.cc \
libs/tinyxml2-11.0.0/tinyxml2.cpp \

LUTC_SRC_PATH := src/entry-points/lutc-main.cc
OWC_SRC_PATH := src/entry-points/owc-main.cc

REMOTE_SRC_PATHS := \
../Firmware/src/system/drivers/save_constructor/save_size.cc \


all_src := $(wildcard $(SRC_PATHS))
all_remote_src := $(wildcard $(REMOTE_SRC_PATHS))

output_dir := build
remote_output_dir := build/remote

objs := $(addprefix $(output_dir)/, $(addsuffix .o, $(basename $(all_src))))
lutc_obj := $(addprefix $(output_dir)/, $(addsuffix .o, $(basename $(LUTC_SRC_PATH))))
owc_obj := $(addprefix $(output_dir)/, $(addsuffix .o, $(basename $(OWC_SRC_PATH))))

remote_objs := $(addprefix $(remote_output_dir)/, $(patsubst ../%,%, $(addsuffix .o, $(basename $(all_remote_src)))))

lutc_output := bin/lutc.exe
owc_output := bin/owc.exe


all: $(lutc_output) $(owc_output)

lutc: $(lutc_output)
owc: $(owc_output)



$(lutc_output): $(objs) $(remote_objs) $(lutc_obj)
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "$(CXX)\t\t$@"
	@$(CXX) $(CXXFLAGS) -o $@ $(objs) $(remote_objs) $(lutc_obj)

$(owc_output): $(objs) $(remote_objs) $(owc_obj)
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "$(CXX)\t\t$@"
	@$(CXX) $(CXXFLAGS) -o $@ $(objs) $(remote_objs) $(owc_obj)

$(output_dir)/%.o: %.cc
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "$(CXX)\t\t$@"
	@$(CXX) $(CXXFLAGS) $(INCLUDE_DIR) -c $< -o $@

$(output_dir)/%.o: %.cpp
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "$(CXX)\t\t$@"
	@$(CXX) $(CXXFLAGS) $(INCLUDE_DIR) -c $< -o $@

$(remote_output_dir)/%.o: ../%.cc
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "$(CXX)(remote)\t$@"
	@$(CXX) $(CXXFLAGS) $(INCLUDE_DIR) -c $< -o $@

$(remote_output_dir)/%.o: ../%.cpp
	@test -d $(dir $@) || mkdir -p $(dir $@)
	@echo -e "$(CXX)(remote)\t$@"
	@$(CXX) $(CXXFLAGS) $(INCLUDE_DIR) -c $< -o $@

.PHONY : clean
clean:
	rm -rf $(lutc_output) $(owc_output) $(output_dir)/ $(remote_output_dir)/
