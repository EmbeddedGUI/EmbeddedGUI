# define source directory
SRC		+= $(EGUI_PORT_PATH)

# define include directory
INCLUDE	+= $(EGUI_PORT_PATH)

# PC port identifier
COMMON_FLAGS += -DEGUI_PORT_PC=1

# define lib directory
LIB		+=

ifeq ($(OS),Windows_NT)
	INCLUDE  += porting/pc/sdl2/$(BITS)/include
	LIB  += porting/pc/sdl2/$(BITS)/lib
	LFLAGS  += -lSDL2main -lSDL2 -lpthread
	LDFLAGS  +=  -Wl,--warn-common
	GCC_BIN_DIR := $(dir $(firstword $(shell where gcc 2>NUL)))
# 	LDFLAGS  +=  -Wl,--gc-sections
	# LTO only when NOGC=0 (release build)
	ifeq ($(NOGC),0)
		COMMON_FLAGS  +=  -flto
		LDFLAGS  +=  -flto
	endif

	USER_COMPILE_TARGETS := output/SDL2.dll output/libwinpthread-1.dll

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


output/SDL2.dll:
	@$(ECHO) Copy SDL2.dll
	@-cmd /c copy $(call FIXPATH, porting\pc\sdl2\$(BITS)\bin\SDL2.dll) $(OUTPUT_PATH)\SDL2.dll

output/libwinpthread-1.dll:
	@$(ECHO) Copy libwinpthread-1.dll
	@-cmd /c copy $(call FIXPATH, $(GCC_BIN_DIR)libwinpthread-1.dll) $(OUTPUT_PATH)\libwinpthread-1.dll


include $(EGUI_PORT_PATH)/Makefile.base
