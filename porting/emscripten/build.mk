# Emscripten port build configuration

# Source: use emscripten-specific main and port, plus PC's SDL backend
SRC += $(EGUI_PORT_PATH)
SRC += porting/pc

# Include: need both emscripten and PC headers
INCLUDE += $(EGUI_PORT_PATH)
INCLUDE += porting/pc

# PC port identifier (reuse PC display/platform abstractions)
COMMON_FLAGS += -DEGUI_PORT_PC=1

# Disable recording test for web builds
COMMON_FLAGS += -DEGUI_CONFIG_RECORDING_TEST=0

# Emscripten compiler - prefer the repo-local emsdk install when present,
# otherwise fall back to environment variables or plain emcc in PATH.
ifneq ($(wildcard tools/emsdk/upstream/emscripten),)
EMSDK_PATH := tools/emsdk
else
EMSDK_PATH ?= $(EMSDK)
endif
ifeq ($(strip $(EMSDK_PATH)),)
CC := emcc
else ifeq ($(OS),Windows_NT)
CC := python scripts/emcc_wrapper.py
else
CC := $(EMSDK_PATH)/upstream/emscripten/emcc
endif

# SDL2 via Emscripten port (needed for both compile and link)
COMMON_FLAGS += -s USE_SDL=2
LFLAGS += -s USE_SDL=2
LFLAGS += -s ALLOW_MEMORY_GROWTH=1
LFLAGS += -s INITIAL_MEMORY=33554432
LFLAGS += -s STACK_SIZE=5242880
LFLAGS += -s EXPORTED_FUNCTIONS='["_main"]'
LFLAGS += -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]'
LFLAGS += --shell-file $(EGUI_PORT_PATH)/shell.html

# OUTPUT_PATH must be defined here (same default as Makefile.emscripten) so the
# wildcard check below resolves before Makefile.emscripten is included.
OUTPUT_PATH ?= output

# Preload resource file into Emscripten virtual filesystem
ifneq ($(wildcard $(OUTPUT_PATH)/app_egui_resource_merge.bin),)
LFLAGS += --preload-file $(OUTPUT_PATH)/app_egui_resource_merge.bin@app_egui_resource_merge.bin
endif

# Output as HTML
TARGET := $(APP).html

# No GC sections for Emscripten
NOGC := 1

include $(EGUI_PORT_PATH)/Makefile.emscripten
