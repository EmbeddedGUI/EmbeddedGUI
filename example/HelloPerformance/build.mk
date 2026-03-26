EGUI_CODE_SRC		+= $(EGUI_APP_PATH)
EGUI_CODE_SRC		+= $(EGUI_APP_PATH)/resource
EGUI_CODE_SRC		+= $(EGUI_APP_PATH)/resource/img
EGUI_CODE_SRC		+= $(EGUI_APP_PATH)/resource/font

EGUI_CODE_INCLUDE	+= $(EGUI_APP_PATH)
EGUI_CODE_INCLUDE	+= $(EGUI_APP_PATH)/resource

ifeq ($(PORT),qemu)
# HelloPerformance uses startup_qemu.s directly and exits through the
# semihosting helper, so rdimon-crt0 only adds unused atexit/init state.
QEMU_USE_RDIMON_STARTFILES ?= 0
endif
