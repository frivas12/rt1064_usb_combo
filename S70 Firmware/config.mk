#
# Copyright (c) 2011 Atmel Corporation. All rights reserved.
#
# \asf_license_start
#
# \page License
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. The name of Atmel may not be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# 4. This software may only be redistributed and used in connection with an
#    Atmel microcontroller product.
#
# THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
# EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# \asf_license_stop
#

# Set up build dir.
BUILD_DIR = build

# Debug elf file (used to make debugging easily configurable).
DEBUG_ELF = 70-0059.elf


# If RTOS-level debugging should be enabled
DEBUG_RTOS = 1

# Path to top level ASF directory relative to this project directory.
PRJ_PATH = ../Firmware

# Target CPU architecture: cortex-m3, cortex-m4
ARCH = cortex-m7

# Target part: none, sam3n4 or sam4l4aa
#PART = samv71q21
PART = sams70n21

# List of C source files.
# Resolves to CSRCS variable after wildcard.
C_SRCS = \
	src/ASF/sam/utils/cmsis/sams70/source/templates/gcc/startup_sams70.c \
	src/ASF/sam/utils/cmsis/sams70/source/templates/system_sams70.c \
	src/ASF/common/services/clock/sams70/sysclk.c \
	src/ASF/common/services/delay/sam/cycle_counter.c \
	src/ASF/common/services/sleepmgr/sam/sleepmgr.c \
	src/ASF/common/services/usb/class/cdc/device/udi_cdc.c \
	src/ASF/common/utils/interrupt/interrupt_sam_nvic.c \
	src/ASF/sam/drivers/mpu/mpu.c \
	src/ASF/sam/drivers/pio/pio.c \
	src/ASF/sam/drivers/pio/pio_handler.c \
	src/ASF/sam/drivers/pmc/pmc.c \
	src/ASF/sam/drivers/pmc/sleep.c \
	src/ASF/sam/drivers/spi/spi.c \
	src/ASF/sam/drivers/afec/afec.c \
	src/ASF/sam/drivers/efc/efc.c \
	src/ASF/sam/drivers/wdt/wdt.c \
	src/ASF/sam/drivers/xdmac/xdmac.c \
	src/ASF/sam/drivers/usbhs/usbhs_device.c \
	src/ASF/sam/drivers/uart/uart.c \
	src/ASF/sam/drivers/usart/usart.c \
	src/ASF/sam/utils/syscalls/gcc/syscalls.c \
	src/ASF/sam/services/flash_efc/flash_efc.c \
	src/ASF/common/services/serial/usart_serial.c \
	src/ASF/common/utils/stdio/read.c \
	src/ASF/common/utils/stdio/write.c \
	src/system/boards/init.c \
	src/system/debug/SEGGER_RTT.c \
 	src/system/debug/SEGGER_RTT_printf.c \
 	src/system/debug/SEGGER_SYSVIEW.c \
 	src/system/debug/SEGGER_SYSVIEW_Config_FreeRTOS.c \
 	src/system/debug/SEGGER_SYSVIEW_FreeRTOS.c \
 	src/system/boards/board.c \
	src/system/slots/slots.c \
	src/system/slots/save_structures.c \
	src/system/task/sys_task.c \
	src/system/drivers/i2c/soft_i2c.c \
	src/system/drivers/eeprom/m24c08.c \
	src/system/drivers/eeprom/25lc1024.c \
	src/system/drivers/usb_slave/usb_slave.c \
	src/system/drivers/usb_slave/ftdi/ftdi.c \
	src/system/drivers/usb_host/*.c \
	src/system/drivers/encoder/encoder.c \
	src/system/drivers/encoder/encoder_quad_linear.c \
	src/system/drivers/encoder/encoder_abs_index_linear.c \
	src/system/drivers/encoder/encoder_abs_biss_linear.c \
	src/system/drivers/encoder/encoder_abs_magnetic_rotation.c \
	src/system/drivers/log/log.c \
	src/system/drivers/buffers/fifo.c \
	src/system/drivers/buffers/ping_pong.c \
	src/system/drivers/limits/usr_limits.c \
	src/system/drivers/one_wire/one_wire.c \
	src/system/drivers/spi/*.c \
	src/system/drivers/cpld/cpld.c \
	src/system/drivers/cpld/cpld_program.c \
	src/system/drivers/adc/user_adc.c \
	src/system/drivers/adc/ads8689.c \
	src/system/drivers/dac/ad56x4.c \
	src/system/drivers/dac/ad5683.c \
	src/system/drivers/interrupt/usr_interrupt.c \
	src/system/drivers/usart/user_usart.c \
	src/system/drivers/io/mcp23s09.c \
	src/system/helper/helper.c \
	src/system/sync/gate/gate.c \
	src/system/boards/hexapod/hexapod.c \
	src/system/boards/hexapod/hex_kins.c \
	src/system/boards/otm/otm.c \
	src/system/cards/stepper/synchronized_motion/synchronized_motion.c \
	src/system/cards/stepper/virtual_motion/virtual_motion.c \
	src/system/drivers/math/matrix_math.c \

	# The following have been omitted as they are linked to build targets:
	# src/ASF/common/services/usb/udc/udc.c 
	# src/ASF/common/services/usb/class/cdc/device/udi_cdc_desc.c

# List of C++ Source Files.
# Resolves to CXXSRCS variable after wildcard.
CXX_SRC = \
	src/system/cards/stepper/*.cc \
	src/system/drivers/usb_host/hid_in.cc \
	src/system/drivers/usb_host/hid_out.cc \

# CXX Source to run through the optimizer
CXX_OPT_SRC = \
	src/system/drivers/supervisor/*.cc \
	src/system/cards/servo/servo.cc \
	src/system/cards/shutter/*.cc \
	src/system/cards/piezo/piezo.cc \
	src/defs.cc \
	src/system/drivers/cpp_alocator/cppmem.cc \
	src/system/drivers/lut/*.cc \
	src/system/drivers/save_constructor/*.cc \
	src/system/drivers/save_constructor/structures/*.cc \
	src/system/cards/flipper-shutter/*.cc \
	src/system/cards/shutter/modules/*.cc \
	src/system/drivers/apt/*.cc \
	src/system/drivers/cpld/*.cc \
	src/system/drivers/eeprom/*.cc \
	src/system/drivers/eeprom/mapper/*.cc \
	src/system/drivers/efs/*.cc \
	src/system/drivers/io/*.cc \
	src/system/drivers/spi/*.cc \
	src/system/drivers/usb_host/hid-mapping-service.cc \
	src/system/drivers/usb_host/hid_item_parser.cc \
	src/system/drivers/usb_host/hid_report.cc \
	src/system/drivers/usb_host/usb-device/*.cc \
	src/system/services/hid-mapping/*.cc \
	src/system/services/itc-service/*.cc \
	src/system/slots/slot_nums.cc \
	src/system/sync/lock_guard/*.cc \
	src/system/sync/rw-lock/*.cc \
	src/main.cc \
	src/system/drivers/device_detect/device_detect.cc \

	# The following have been omitted as they are linked to build targets:
	# src/system/drivers/usb_slave/apt_parse.cc \


FREERTOS_VER=9

ifeq ($(FREERTOS_VER), 10)
C_SRCS += \
	src/ASF/thirdparty/freertos/freertos-10.0.0/Source/croutine.c \
	src/ASF/thirdparty/freertos/freertos-10.0.0/Source/event_groups.c \
	src/ASF/thirdparty/freertos/freertos-10.0.0/Source/list.c \
	src/ASF/thirdparty/freertos/freertos-10.0.0/Source/queue.c \
	src/ASF/thirdparty/freertos/freertos-10.0.0/Source/tasks.c \
	src/ASF/thirdparty/freertos/freertos-10.0.0/Source/timers.c \
	src/ASF/thirdparty/freertos/freertos-10.0.0/Source/portable/GCC/ARM_CM7/r0p1/port.c \
	src/ASF/thirdparty/freertos/freertos-10.0.0/Source/portable/MemMang/heap_3.c \
	src/ASF/thirdparty/freertos/freertos-10.0.0/Source/stream_buffer.c \

endif

ifeq ($(FREERTOS_VER), 9)
	C_SRCS += \
	src/ASF/thirdparty/freertos/freertos-9.0.0/Source/croutine.c \
	src/ASF/thirdparty/freertos/freertos-9.0.0/Source/event_groups.c \
	src/ASF/thirdparty/freertos/freertos-9.0.0/Source/list.c \
	src/ASF/thirdparty/freertos/freertos-9.0.0/Source/queue.c \
	src/ASF/thirdparty/freertos/freertos-9.0.0/Source/tasks.c \
	src/ASF/thirdparty/freertos/freertos-9.0.0/Source/timers.c \
	src/ASF/thirdparty/freertos/freertos-9.0.0/Source/portable/GCC/ARM_CM7/r0p1/port.c \
	src/ASF/thirdparty/freertos/freertos-9.0.0/Source/portable/MemMang/heap_3.c \

endif


# List of assembler source files.
ASSRCS = 

# List of include paths.
INC_PATH = \
 	src \
	src/ASF/sam/drivers/usart \
 	src/ASF/sam/drivers/pmc \
 	src/ASF/sam/drivers/usbhs \
 	src/ASF/sam/drivers/pio \
 	src/ASF/sam/drivers/mpu \
 	src/ASF/sam/drivers/spi \
 	src/ASF/sam/drivers/afec \
 	src/ASF/sam/drivers/wdt \
 	src/ASF/sam/drivers/xdmac \
 	src/ASF/sam/drivers/efc \
 	src/ASF/sam/utils \
 	src/ASF/sam/utils/header_files \
 	src/ASF/sam/utils/preprocessor \
	src/ASF/sam/utils/cmsis/sams70/include \
	src/ASF/sam/utils/cmsis/sams70/include/instance \
 	src/ASF/sam/utils/cmsis/sams70/source/templates \
 	src/ASF/sam/utils/fpu \
 	src/ASF/sam/services \
 	src/ASF/sam/services/flash_efc \
 	src/ASF/thirdparty/CMSIS/Include \
 	src/ASF/thirdparty/CMSIS/DSP/DSP_Lib_TestSuite/RefLibs/inc \
 	src/ASF/thirdparty/CMSIS/Lib/GCC \
 	src/ASF/common/utils \
 	src/ASF/common/services/clock \
 	src/ASF/common/services/delay \
 	src/ASF/common/services/sleepmgr \
 	src/ASF/common/services/usb/class/cdc/device \
 	src/ASF/common/services/usb/class/cdc \
 	src/ASF/common/services/usb/udc \
 	src/ASF/common/services/ioport \
 	src/ASF/common/services/usb \
 	src/ASF/common/boards \
 	src/ASF/common/services/gpio \
 	src/ASF/common/services/ioport/ \
 	src/ASF/common/services/serial/sam_uart	\
	src/ASF/common/services/serial \
 	src/config \
	src/ASF/thirdparty/CMSIS/Core/Include/	\
	src/ASF/thirdparty/CMSIS/DSP/Include/	\
 	src/system/debug \
 	src/system/boards \
 	src/system/boards/hexapod \
 	src/system/boards/otm \
 	src/system/slots \
	src/system/task \
	src/system/drivers/apt \
	src/system/drivers/eeprom \
	src/system/drivers/eeprom/mapper \
	src/system/drivers/efs \
	src/system/drivers/i2c \
	src/system/drivers/spi \
	src/system/drivers/cpld \
	src/system/drivers/adc \
	src/ASF/sam/drivers/uart \
	src/ASF/sam/drivers/usart \
	src/system/drivers/supervisor \
	src/system/drivers/usart \
	src/system/drivers/usb_slave \
	src/system/drivers/usb_slave/ftdi \
	src/system/drivers/usb_host \
	src/system/drivers/usb_host/usb-device \
	src/system/drivers/encoder \
	src/system/drivers/log \
	src/system/drivers/buffers \
	src/system/drivers/limits \
	src/system/drivers/math \
	src/system/drivers/one_wire \
	src/system/drivers/dac \
	src/system/drivers/interrupt \
	src/system/drivers/device_detect \
	src/system/drivers/io \
	src/system/drivers/lut \
	src/system/drivers/save_constructor \
	src/system/drivers/save_constructor/structures/ \
	src/system/drivers/cpp_allocator \
	src/system/helper \
 	src/system/cards \
	src/system/cards/flipper-shutter \
	src/system/cards/stepper \
	src/system/cards/stepper/synchronized_motion \
	src/system/cards/stepper/virtual_motion \
	src/system/cards/servo \
	src/system/cards/shutter \
	src/system/cards/shutter/modules \
	src/system/cards/piezo \
	src/system/services/ \
	src/system/services/hid-mapping \
	src/system/services/itc-service \
	src/system/sync \
	src/system/sync/gate \
	src/system/sync/lock_guard \
	src/system/sync/rw-lock \
	../../../thorlabs_software_lib/ \

ifeq ($(FREERTOS_VER), 10)
INC_PATH += \
	src/ASF/thirdparty/freertos/freertos-10.0.0/Source/include \
	src/ASF/thirdparty/freertos/freertos-10.0.0/Source/portable/GCC/ARM_CM7/r0p1/ \

endif

ifeq ($(FREERTOS_VER), 9)
INC_PATH += \
	src/ASF/thirdparty/freertos/freertos-9.0.0/Source/include \
	src/ASF/thirdparty/freertos/freertos-9.0.0/Source/portable/GCC/ARM_CM7/r0p1/ \

endif

	
#	../../../../../../Progra~2/Atmel/Studio/7.0/packs/atmel/SAMS70_DFP/2.2.81/sams70a/include \
#	/src/ASF/common/utils/stdio/stdio_serial \
		
# Additional search paths for libraries.
LIB_PATH =  \
	src/ASF/thirdparty/CMSIS/Lib/GCC
	
# List of libraries to use during linking.
LIBS = \
	m arm_cortexM7l_math arm_cortexM7lfsp_math                                  
#       arm_cortexM7lfsp_math_softfp                       \

# Path relative to top level directory pointing to a linker script.
LINKER_SCRIPT_FLASH = src/ASF/sam/utils/linker_scripts/sams70/sams70n21/gcc/flash.ld
#LINKER_SCRIPT_SRAM  = src/Device_Startup/samv71q21_sram.ld

# Path relative to top level directory pointing to a linker script.
#DEBUG_SCRIPT_FLASH = sam/boards/samv71_xplained_ultra/debug_scripts/gcc/samv71_xplained_ultra_flash.gdb
#DEBUG_SCRIPT_SRAM  = sam/boards/samv71_xplained_ultra/debug_scripts/gcc/samv71_xplained_ultra_sram.gdb

# Project type parameter: all, sram or flash
PROJECT_TYPE        = flash

# Additional options for debugging. By default the common Makefile.in will
# add -g3. -O0 for no debug
DBGFLAGS = -g3
# No debug

# Application optimization used during compilation and linking:
# -O0, -O1, -O2, -O3 or -Os use -O0
ifdef USE_OPTIMIZED_DEBUG
	OPTIMIZATION_DEBUG = -Og
else
	OPTIMIZATION_DEBUG = -O0
endif
# No debug
OPTIMIZATION_RELEASE = -Os

# Stepper log status flags
ifdef ENABLE_STEPPER_LOG_STATUS_FLAGS
	ENABLE_STEPPER_LOG_STATUS_FLAGS := 1
else
	ENABLE_STEPPER_LOG_STATUS_FLAGS := 0
endif


       
# Extra flags to use when assembling.
ASFLAGS = \ 
		-std=gnu99\
		-mcpu=cortex-m7 -mthumb\
		-mfloat-abi=softfp\
		-mfpu=fpv5-sp-d16\

# Extra flags to use when compiling.
CFLAGS = \
		-mcpu=cortex-m7 -mthumb\
		-mfloat-abi=softfp\
		-mfpu=fpv5-sp-d16\
		-D ENABLE_STEPPER_LOG_STATUS_FLAGS=$(ENABLE_STEPPER_LOG_STATUS_FLAGS)\

# Extra flags to use when preprocessing.
#
# Preprocessor symbol definitions
#   To add a definition use the format "-D name[=definition]".
#   To cancel a definition use the format "-U name".
#
# The most relevant symbols to define for the preprocessor are:
#   BOARD      Target board in use, see boards/board.h for a list.
#   EXT_BOARD  Optional extension board in use, see boards/board.h for a list.
CPPFLAGS = \
       	-D ARM_MATH_CM7=true\
       	-D BOARD=USER_BOARD\
       	-D __SAMS70N21__\
       	-D __FPU_PRESENT=1\
		-D ENABLE_STEPPER_LOG_STATUS_FLAGS=$(ENABLE_STEPPER_LOG_STATUS_FLAGS)\
		

ifdef STACK_TREE
	CFLAGS += -fdump-rtl-expand
	CPPFLAGS += -fdump-rtl-expand
endif

ifdef STACK_USAGE
	CFLAGS += -fstack-usage
	CPPFLAGS += -fstack-usage
endif

# Extra flags to use when linking
LDFLAGS = \
		-Wl,--gc-sections\
		-Wl,--start-group -lm\
		-Wl,--end-group\
		-Wl,--print-memory-usage\
		-mthumb -Wl,-Map="s70_4.map" --specs=nano.specs
		-mcpu=cortex-m7 -mthumb\
		-mfloat-abi=softfp\
		-mfpu=fpv5-sp-d16\
		
	
# Pre- and post-build commands
PREBUILD_CMD = 
POSTBUILD_CMD = 

# Path to the C++ Makefile template (for the template Makefile preprocessor).
CPP_TEMPLATE:=CXXMakeTemplate.mk

# Resolving wildcards in source paths.
CSRCS = $(wildcard $(C_SRCS))
CXXSRC = $(wildcard $(CXX_SRC))
CXXOPTSRC = $(wildcard $(CXX_OPT_SRC))
