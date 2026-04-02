# define source directory
SRC		+= $(EGUI_PORT_PATH)

# define include directory
INCLUDE	+= $(EGUI_PORT_PATH)

# driver file selection is port-owned
EGUI_CODE_SRC_FILES += \
    driver/lcd/egui_lcd.c

# define lib directory
LIB		+=

ifeq ($(OS),Windows_NT)
	LDFLAGS  +=  -Wl,--warn-common
	USER_COMPILE_TARGETS :=
else ifeq ($(shell uname), Darwin)
	NOGC := 1
else
	LDFLAGS  +=  -Wl,--warn-common
endif

# No SDL2 libraries needed for headless test port
LFLAGS  += -lm

include $(EGUI_PORT_PATH)/Makefile.base
