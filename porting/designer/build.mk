# Headless designer port - no SDL, pipe-based IPC

# define source directory
SRC		+= $(EGUI_PORT_PATH)

# define include directory
INCLUDE	+= $(EGUI_PORT_PATH)

# driver file selection is port-owned
EGUI_CODE_SRC_FILES += \
    driver/lcd/egui_lcd.c \
    driver/touch/egui_touch.c

# PC port identifier (reuse PC color depth config)
COMMON_FLAGS += -DEGUI_PORT_PC=1
COMMON_FLAGS += -DEGUI_PORT_DESIGNER=1

# Designer outputs log to stderr to preserve stdout IPC pipe
COMMON_FLAGS += -DEGUI_CONFIG_PLATFORM_CUSTOM_PRINTF=1

# define lib directory
LIB		+=

ifeq ($(OS),Windows_NT)
	LFLAGS  += -lpthread
	LDFLAGS  +=  -Wl,--warn-common
	ifeq ($(NOGC),0)
		COMMON_FLAGS  +=  -flto
		LDFLAGS  +=  -flto
	endif
else ifeq ($(shell uname), Darwin)
	LFLAGS += -lpthread
	NOGC := 1
else
	LFLAGS += -lpthread
	LDFLAGS +=  -Wl,--warn-common -Wl,--gc-sections
endif

include $(EGUI_PORT_PATH)/Makefile.base
