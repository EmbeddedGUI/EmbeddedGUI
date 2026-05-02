EGUI_CODE_SRC += $(EGUI_APP_PATH)

EGUI_CODE_INCLUDE += $(EGUI_APP_PATH)

# select the sub app
APP_SUB ?= snake

EGUI_APP_SUB_PATH := $(EGUI_APP_PATH)/$(APP_SUB)

# Each sub-app has its own app_egui_config.h, so framework sources need
# separate objects per game configuration.
APP_OBJ_SUFFIX := HelloGame_$(APP_SUB)

EGUI_CODE_SRC += $(EGUI_APP_SUB_PATH)

EGUI_CODE_INCLUDE += $(EGUI_APP_SUB_PATH)

