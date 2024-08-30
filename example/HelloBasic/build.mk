SRC		+= $(APP_PATH)

INCLUDE	+= $(APP_PATH)


# select the sub app
# APP_SUB ?= anim
# APP_SUB ?= button
# APP_SUB ?= button_img
# APP_SUB ?= image
# APP_SUB ?= label
# APP_SUB ?= linearlayout
# APP_SUB ?= mask
APP_SUB ?= progress_bar
# APP_SUB ?= scroll
# APP_SUB ?= switch
# APP_SUB ?= viewpage

APP_SUB_PATH := $(APP_PATH)/$(APP_SUB)

SRC		+= $(APP_SUB_PATH)
SRC		+= $(APP_SUB_PATH)/resource

INCLUDE	+= $(APP_SUB_PATH)
