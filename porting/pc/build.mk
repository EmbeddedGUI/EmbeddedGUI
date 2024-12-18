# define source directory
SRC		+= $(EGUI_PORT_PATH)

# define include directory
INCLUDE	+= $(EGUI_PORT_PATH)

# define lib directory
LIB		+=

BITS ?= 32

ifeq ($(OS),Windows_NT)
	INCLUDE  += porting/pc/sdl2/$(BITS)/include
	LIB  += porting/pc/sdl2/$(BITS)/lib
	LFLAGS  += -lSDL2main -lSDL2 -lpthread
	COMMON_FLAGS  +=  -flto
	LDFLAGS  +=  -Wl,--warn-common -flto
	LDFLAGS  +=  -Wl,--gc-sections

	USER_COMPILE_TARGETS := output/SDL2.dll

else ifeq ($(shell uname), Darwin)
    # Code for OS X
	COMMON_FLAGS  += -DSDL_DISABLE_ARM_NEON_H -D_THREAD_SAFE
	INCLUDE  += /opt/homebrew/include
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


include $(EGUI_PORT_PATH)/Makefile.base
