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
# The current stack report keeps all active HelloPerformance paths below 1KB
# per frame and below 1.2KB even in compiled but non-recorded code paths, so
# a 4KB QEMU stack reserve keeps multi-frame headroom without carrying the
# default 8KB static RAM cost in `._user_heap_stack`.
APP_LDFLAGS += -Wl,--defsym=__qemu_min_stack_size__=0x1000
endif
