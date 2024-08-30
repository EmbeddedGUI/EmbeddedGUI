# define source directory
SRC		+= $(PORT_PATH)
SRC		+= $(PORT_PATH)/Core/Src
SRC		+= $(PORT_PATH)/Core/Startup
SRC		+= $(PORT_PATH)/Porting

# define include directory
INCLUDE	+= $(PORT_PATH)
INCLUDE	+= $(PORT_PATH)/Core/Inc

# define lib directory
LIB		+=






GCC_DIR = $(PORT_PATH)/GCC

ROOT_DIR = $(PORT_PATH)

LIB_DIR = $(ROOT_DIR)/Drivers

CMSIS_DIR = $(LIB_DIR)/CMSIS
CMSIS_DEVICE_DIR = $(CMSIS_DIR)/Device/ST/STM32G0xx
BSP_DIR = $(LIB_DIR)/BSP
DRIVER_DIR = $(LIB_DIR)/STM32G0xx_HAL_Driver




# select CPU
COMMON_FLAGS  += -DUSE_HAL_DRIVER
COMMON_FLAGS  += -DSTM32G0B0xx
COMMON_FLAGS  += -DEGUI_CONFIG_COLOR_16_SWAP=1

CPU_ARCH := cortex-m0plus
JLINK_DEVICE  := STM32G0B0RE

LINKER_SCRIPT ?= $(PORT_PATH)/STM32G0B0RETX_FLASH.ld

include $(PORT_PATH)/GCC/Makefile.base
