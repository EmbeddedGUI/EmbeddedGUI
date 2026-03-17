EGUI_CODE_SRC		+= $(EGUI_APP_PATH)
EGUI_CODE_SRC		+= $(EGUI_APP_PATH)/resource
EGUI_CODE_SRC		+= $(EGUI_APP_PATH)/resource/font
EGUI_CODE_SRC		+= $(EGUI_APP_PATH)/resource/img

EGUI_CODE_INCLUDE	+= $(EGUI_APP_PATH)
EGUI_CODE_INCLUDE	+= $(EGUI_APP_PATH)/resource

# Large screen (1280x1024) needs bigger stack for snap_shot RGB buffer
ifeq ($(PORT),emscripten)
LFLAGS += -s STACK_SIZE=8388608
else ifeq ($(PORT),pc)
ifeq ($(OS),Windows_NT)
LDFLAGS += -Wl,--stack,8388608
else
LDFLAGS += -Wl,-z,stacksize=8388608
endif
endif
