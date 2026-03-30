# QEMU ARM port build configuration
# Usage: make all APP=HelloPerformance PORT=qemu CPU_ARCH=cortex-m3

# define source directory
SRC		+= $(EGUI_PORT_PATH)

# define include directory
INCLUDE	+= $(EGUI_PORT_PATH)

# define lib directory
LIB		+=

# CPU architecture (overridable from command line)
CPU_ARCH ?= cortex-m3

# Linker script
LINKER_SCRIPT ?= $(EGUI_PORT_PATH)/qemu.ld

# Use semihosting for I/O. Keep qemu size baselines aligned with stm32g0_empty
# by using newlib-nano instead of the full libc.
# Most QEMU apps keep the rdimon startfiles, but apps with their own startup
# and no constructor/atexit needs can disable them to trim startup state.
QEMU_USE_RDIMON_STARTFILES ?= 1

ifeq ($(QEMU_USE_RDIMON_STARTFILES),0)
STDCLIB_LDFLAGS = --specs=nano.specs --specs=rdimon.specs -nostartfiles -lc -lrdimon -lgcc
else
STDCLIB_LDFLAGS = --specs=nano.specs --specs=rdimon.specs -lc -lrdimon -lgcc
endif

# No HAL drivers needed
GCC_DIR = $(EGUI_PORT_PATH)/GCC

include $(EGUI_PORT_PATH)/GCC/Makefile.base
