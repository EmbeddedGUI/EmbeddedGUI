EGUI_CODE_SRC		+= $(EGUI_APP_PATH)

EGUI_CODE_INCLUDE	+= $(EGUI_APP_PATH)


# select the sub app
APP_SUB ?= virtual_viewport_basic
# APP_SUB ?= virtual_viewport
# APP_SUB ?= virtual_page_basic
# APP_SUB ?= virtual_page
# APP_SUB ?= virtual_strip_basic
# APP_SUB ?= virtual_strip
# APP_SUB ?= virtual_grid_basic
# APP_SUB ?= virtual_grid
# APP_SUB ?= list_view_basic
# APP_SUB ?= list_view
# APP_SUB ?= grid_view_basic
# APP_SUB ?= grid_view
# APP_SUB ?= virtual_section_list_basic
# APP_SUB ?= virtual_section_list
# APP_SUB ?= virtual_tree_basic
# APP_SUB ?= virtual_tree
# APP_SUB ?= virtual_stage_basic
# APP_SUB ?= virtual_stage_showcase
# APP_SUB ?= virtual_stage

EGUI_APP_SUB_PATH := $(EGUI_APP_PATH)/$(APP_SUB)

# Each sub-app has its own app_egui_config.h, so they need separate OBJDIRs
# to ensure framework sources are compiled with the correct config flags.
APP_OBJ_SUFFIX := HelloVirtual_$(APP_SUB)

EGUI_CODE_SRC		+= $(EGUI_APP_SUB_PATH)
EGUI_CODE_SRC		+= $(EGUI_APP_SUB_PATH)/resource
EGUI_CODE_SRC		+= $(EGUI_APP_SUB_PATH)/resource/img
EGUI_CODE_SRC		+= $(EGUI_APP_SUB_PATH)/resource/font

EGUI_CODE_INCLUDE	+= $(EGUI_APP_SUB_PATH)
EGUI_CODE_INCLUDE	+= $(EGUI_APP_SUB_PATH)/resource


EGUI_APP_RESOURCE_PATH ?= $(EGUI_APP_SUB_PATH)/resource
