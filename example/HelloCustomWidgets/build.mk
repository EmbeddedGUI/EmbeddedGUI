EGUI_CODE_SRC		+= $(EGUI_APP_PATH)

EGUI_CODE_INCLUDE	+= $(EGUI_APP_PATH)


# select the sub app
APP_SUB ?= input/xy_pad

EGUI_APP_SUB_PATH := $(EGUI_APP_PATH)/$(APP_SUB)

# Each sub-app has its own obj directory (/ replaced with _)
APP_OBJ_SUFFIX := HelloCustomWidgets_$(subst /,_,$(APP_SUB))

EGUI_CODE_SRC		+= $(EGUI_APP_SUB_PATH)
EGUI_CODE_SRC		+= $(EGUI_APP_SUB_PATH)/resource
EGUI_CODE_SRC		+= $(EGUI_APP_SUB_PATH)/resource/img
EGUI_CODE_SRC		+= $(EGUI_APP_SUB_PATH)/resource/font

EGUI_CODE_INCLUDE	+= $(EGUI_APP_SUB_PATH)
EGUI_CODE_INCLUDE	+= $(EGUI_APP_SUB_PATH)/resource

EGUI_APP_RESOURCE_PATH ?= $(EGUI_APP_SUB_PATH)/resource
