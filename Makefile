# include app info
# APP ?= HelloActivity
# APP ?= HelloAPP
# APP ?= HelloBasic
# APP ?= HelloEasyPage
# APP ?= HelloPerformace
# APP ?= HelloResourceManager
APP ?= HelloSimple
# APP ?= HelloTest
# APP ?= HelloUnitTest
# APP ?= HelloViewPageAndScroll
# APP ?= gui

# include port info
PORT ?= pc
# PORT ?= stm32g0
# PORT ?= stm32g0_empty

# set special cflag, if need.
# COMPILE_OPT_LEVEL ?= -O0
COMPILE_OPT_LEVEL ?= -O2
# COMPILE_OPT_LEVEL ?= -Os
COMPILE_DEBUG ?= -g

# show compile debug info.
# V := 1

# BITS is auto-detected from compiler target in porting/*/build.mk
# Override manually if needed: make BITS=32

# For user show current app info.
COMMON_FLAGS += -DEGUI_APP=\"$(APP)\"
ifeq ($(PORT),pc)
COMMON_FLAGS += -DEGUI_PORT=EGUI_PORT_TYPE_PC
else ifeq ($(PORT),designer)
COMMON_FLAGS += -DEGUI_PORT=EGUI_PORT_TYPE_PC
else ifeq ($(PORT),qemu)
COMMON_FLAGS += -DEGUI_PORT=EGUI_PORT_TYPE_QEMU
else ifeq ($(PORT),emscripten)
COMMON_FLAGS += -DEGUI_PORT=EGUI_PORT_TYPE_PC
else
COMMON_FLAGS += -DEGUI_PORT=EGUI_PORT_TYPE_MCU
endif


# Set default toolchain paths, can be overridden by environment variables or command line.
ARM_GCC_PATH ?= D:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/12.2 mpacbti-rel1
QEMU_PATH ?= C:/Program Files/qemu/



EGUI_CODE_SRC := 
EGUI_CODE_INCLUDE := 

# don't edit below this line.
# include egui src path (defined early so app build.mk can reference it)
EGUI_PATH := src

# include app info
EGUI_APP_ROOT_PATH ?= example
# EGUI_APP_ROOT_PATH ?= ..
EGUI_APP_PATH = $(EGUI_APP_ROOT_PATH)/$(APP)
include $(EGUI_APP_PATH)/build.mk
# set app resource path
EGUI_APP_RESOURCE_PATH ?= $(EGUI_APP_PATH)/resource



# include egui src
include $(EGUI_PATH)/build.mk

# include driver layer (HAL)
EGUI_DRIVER_PATH := driver
include $(EGUI_DRIVER_PATH)/build.mk

SRC += $(EGUI_CODE_SRC)
INCLUDE += $(EGUI_CODE_INCLUDE)

# include port info
EGUI_PORT_ROOT_PATH = porting
EGUI_PORT_PATH = $(EGUI_PORT_ROOT_PATH)/$(PORT)
include $(EGUI_PORT_PATH)/build.mk
