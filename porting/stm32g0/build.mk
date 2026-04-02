# define source directory
SRC		+= $(EGUI_PORT_PATH)
SRC		+= $(EGUI_PORT_PATH)/Core/Src
SRC		+= $(EGUI_PORT_PATH)/Core/Startup
SRC		+= $(EGUI_PORT_PATH)/Porting

# define include directory
INCLUDE	+= $(EGUI_PORT_PATH)
INCLUDE	+= $(EGUI_PORT_PATH)/Core/Inc
INCLUDE	+= $(EGUI_PORT_PATH)/Porting

# driver file selection is port-owned
EGUI_CODE_SRC_FILES += \
    driver/lcd/egui_lcd.c \
    driver/touch/egui_touch.c \
    driver/bus/egui_panel_io_spi.c \
    driver/bus/egui_panel_io_i2c.c \
    driver/lcd/egui_lcd_st7789.c \
    driver/touch/egui_touch_ft6336.c

COMMON_FLAGS += \
    -DEGUI_DRIVER_PANEL_IO_SPI_ENABLE=1 \
    -DEGUI_DRIVER_PANEL_IO_I2C_ENABLE=1 \
    -DEGUI_DRIVER_LCD_ST7789_ENABLE=1 \
    -DEGUI_DRIVER_TOUCH_FT6336_ENABLE=1

# define lib directory
LIB		+=






GCC_DIR = $(EGUI_PORT_PATH)/GCC

ROOT_DIR = $(EGUI_PORT_PATH)

LIB_DIR = $(ROOT_DIR)/Drivers

CMSIS_DIR = $(LIB_DIR)/CMSIS
CMSIS_DEVICE_DIR = $(CMSIS_DIR)/Device/ST/STM32G0xx
BSP_DIR = $(LIB_DIR)/BSP
DRIVER_DIR = $(LIB_DIR)/STM32G0xx_HAL_Driver




# select CPU
COMMON_FLAGS  += -DUSE_HAL_DRIVER
COMMON_FLAGS  += -DSTM32G0B0xx

CPU_ARCH := cortex-m0plus
JLINK_DEVICE  := STM32G0B0RE

LINKER_SCRIPT ?= $(EGUI_PORT_PATH)/STM32G0B0RETX_FLASH.ld

include $(EGUI_PORT_PATH)/GCC/Makefile.base
