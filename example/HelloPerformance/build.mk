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
# The current linked HelloPerformance stack hotspot is 752B per frame, while
# the larger 1.1-1.2KB stack frames belong to discarded widget code in this
# app image, so a 768B QEMU stack reserve is the smallest validated reserve
# that still lets the full benchmark path run without wasting more static RAM
# in `._user_heap_stack`.
APP_LDFLAGS += -Wl,--defsym=__qemu_min_stack_size__=0x0300
endif
