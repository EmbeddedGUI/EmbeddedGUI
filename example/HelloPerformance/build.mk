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
# HelloPerformance uses the self-managed QEMU heap, so the linker-side
# reserved heap budget can stay at 0.
APP_LDFLAGS += -Wl,--defsym=__qemu_min_heap_size__=0
# The current linked HelloPerformance stack hotspot is 432B per frame after the
# latest function-local stack trims, while the larger 976B / 1112B / 1200B
# frames still belong to discarded widget code in this app image. Keep the
# QEMU reserve just above the active path instead of wasting more static RAM in
# `._user_heap_stack`.
APP_LDFLAGS += -Wl,--defsym=__qemu_min_stack_size__=0x01b0
endif
