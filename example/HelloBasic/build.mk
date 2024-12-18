EGUI_CODE_SRC		+= $(EGUI_APP_PATH)

EGUI_CODE_INCLUDE	+= $(EGUI_APP_PATH)


# select the sub app
# EGUI_APP_SUB ?= anim
EGUI_APP_SUB ?= button
# EGUI_APP_SUB ?= button_img
# EGUI_APP_SUB ?= image
# EGUI_APP_SUB ?= label
# EGUI_APP_SUB ?= linearlayout
# EGUI_APP_SUB ?= mask
# EGUI_APP_SUB ?= progress_bar
# EGUI_APP_SUB ?= scroll
# EGUI_APP_SUB ?= switch
# EGUI_APP_SUB ?= viewpage
# EGUI_APP_SUB ?= viewpage_cache

EGUI_APP_SUB_PATH := $(EGUI_APP_PATH)/$(EGUI_APP_SUB)

EGUI_CODE_SRC		+= $(EGUI_APP_SUB_PATH)
EGUI_CODE_SRC		+= $(EGUI_APP_SUB_PATH)/resource

EGUI_CODE_INCLUDE	+= $(EGUI_APP_SUB_PATH)
