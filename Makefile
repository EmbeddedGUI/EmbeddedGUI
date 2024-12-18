# include app info
# APP ?= HelloActivity
# APP ?= HelloBasic
# APP ?= HelloPerformace
# APP ?= HelloResourceManager
APP ?= HelloSimple
# APP ?= HelloTest
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

# for 64bit system
# BITS=64

# For user show current app info.
COMMON_FLAGS += -DEGUI_APP=\"$(APP)\"
ifeq ($(PORT),pc)
COMMON_FLAGS += -DEGUI_PORT=EGUI_PORT_TYPE_PC
else
COMMON_FLAGS += -DEGUI_PORT=EGUI_PORT_TYPE_MCU
endif








EGUI_CODE_SRC := 
EGUI_CODE_INCLUDE := 

# don't edit below this line.
# include app info
EGUI_APP_ROOT_PATH ?= example
EGUI_APP_PATH = $(EGUI_APP_ROOT_PATH)/$(APP)
include $(EGUI_APP_PATH)/build.mk

# include egui src
EGUI_PATH := src
include $(EGUI_PATH)/build.mk

SRC += $(EGUI_CODE_SRC)
INCLUDE += $(EGUI_CODE_INCLUDE)

# include port info
EGUI_PORT_ROOT_PATH = porting
EGUI_PORT_PATH = $(EGUI_PORT_ROOT_PATH)/$(PORT)
include $(EGUI_PORT_PATH)/build.mk
