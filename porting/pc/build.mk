# define source directory
SRC		+= $(EGUI_PORT_PATH)

# define include directory
INCLUDE	+= $(EGUI_PORT_PATH)

# driver file selection is port-owned
EGUI_CODE_SRC_FILES += \
    driver/lcd/egui_lcd.c \
    driver/touch/egui_touch.c

# PC port identifier
COMMON_FLAGS += -DEGUI_PORT_PC=1

# define lib directory
LIB		+=
OUTPUT_PATH ?= output

ifeq ($(OS),Windows_NT)
	INCLUDE  += porting/pc/sdl2/$(BITS)/include
	LIB  += porting/pc/sdl2/$(BITS)/lib
	LFLAGS  += -lSDL2main -lSDL2 -lpthread
	LDFLAGS  +=  -Wl,--warn-common
	GCC_BIN_DIR := $(dir $(firstword $(shell where gcc 2>NUL)))
	LIBWINPTHREAD_DLL := $(wildcard $(GCC_BIN_DIR)libwinpthread-1.dll)
# 	LDFLAGS  +=  -Wl,--gc-sections
	# LTO only when NOGC=0 (release build)
	ifeq ($(NOGC),0)
		COMMON_FLAGS  +=  -flto
		LDFLAGS  +=  -flto
	endif

	USER_COMPILE_TARGETS := $(OUTPUT_PATH)/SDL2.dll
	ifneq ($(strip $(LIBWINPTHREAD_DLL)),)
		USER_COMPILE_TARGETS += $(OUTPUT_PATH)/libwinpthread-1.dll
	endif

else ifeq ($(shell uname), Darwin)
    # Code for OS X
# COMMON_FLAGS  += -DSDL_DISABLE_ARM_NEON_H -D_THREAD_SAFE
# INCLUDE  += /opt/homebrew/include
# LIB  += /opt/homebrew/lib
	LFLAGS  += -lSDL2 -lSDL2main

	NOGC := 1
else

	COMMON_FLAGS  +=  
	LFLAGS  += -lSDL2
	LDFLAGS  +=  -Wl,--warn-common
	LDFLAGS  +=  -Wl,--gc-sections
endif


$(OUTPUT_PATH)/SDL2.dll: | $(OUTPUT_PATH)
	@$(ECHO) Copy SDL2.dll
	@-cmd /c copy $(call FIXPATH, porting\pc\sdl2\$(BITS)\bin\SDL2.dll) $(call FIXPATH, $@)

$(OUTPUT_PATH)/libwinpthread-1.dll: | $(OUTPUT_PATH)
	@$(ECHO) Copy libwinpthread-1.dll
	@-cmd /c copy $(call FIXPATH, $(LIBWINPTHREAD_DLL)) $(call FIXPATH, $@)


include $(EGUI_PORT_PATH)/Makefile.base
