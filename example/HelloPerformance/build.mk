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
# HelloPerformance keeps QEMU platform malloc disabled and current runtime
# verification shows zero heap usage, so the app can drop QEMU's default
# reserved heap budget while keeping the standard stack headroom.
APP_LDFLAGS += -Wl,--defsym=__qemu_min_heap_size__=0
endif
