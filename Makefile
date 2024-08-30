# include app info
# APP ?= HelloActivity
# APP ?= HelloBasic
# APP ?= HelloPerformace
# APP ?= HelloSimple
APP ?= HelloTest
# APP ?= HelloViewPageAndScroll

# include port info
PORT ?= pc
# PORT ?= stm32g0
# PORT ?= stm32g0_empty

# set special cflag, if need.
COMMON_FLAGS  := -O2
# COMMON_FLAGS  := -Os

# show compile debug info.
# V := 1

# For user show current app info.
COMMON_FLAGS += -DEGUI_APP=\"$(APP)\"
ifeq ($(PORT),pc)
COMMON_FLAGS += -DEGUI_PORT=EGUI_PORT_TYPE_PC
else
COMMON_FLAGS += -DEGUI_PORT=EGUI_PORT_TYPE_MCU
endif










# don't edit below this line.
# include app info
APP_ROOT_PATH = example
APP_PATH = $(APP_ROOT_PATH)/$(APP)
include $(APP_PATH)/build.mk

# include egui src
EGUI_PATH := src
include $(EGUI_PATH)/build.mk


# include port info
PORT_ROOT_PATH = porting
PORT_PATH = $(PORT_ROOT_PATH)/$(PORT)
include $(PORT_PATH)/build.mk
