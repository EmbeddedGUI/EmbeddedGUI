# QEMU ARM port build configuration
# Usage: make all APP=HelloPerformace PORT=qemu CPU_ARCH=cortex-m3

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

# Use semihosting for I/O (rdimon provides printf via semihosting)
STDCLIB_LDFLAGS = --specs=rdimon.specs -lc -lrdimon -lgcc

# No HAL drivers needed
GCC_DIR = $(EGUI_PORT_PATH)/GCC

include $(EGUI_PORT_PATH)/GCC/Makefile.base
