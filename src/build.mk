EGUI_CODE_SRC		+= $(EGUI_PATH)
EGUI_CODE_SRC		+= $(EGUI_PATH)/app
EGUI_CODE_SRC		+= $(EGUI_PATH)/anim
EGUI_CODE_SRC		+= $(EGUI_PATH)/background
EGUI_CODE_SRC		+= $(EGUI_PATH)/canvas
EGUI_CODE_SRC		+= $(EGUI_PATH)/core
EGUI_CODE_SRC		+= $(EGUI_PATH)/widget
EGUI_CODE_SRC		+= $(EGUI_PATH)/font
EGUI_CODE_SRC		+= $(EGUI_PATH)/image
EGUI_CODE_SRC		+= $(EGUI_PATH)/mask
EGUI_CODE_SRC		+= $(EGUI_PATH)/shadow
EGUI_CODE_SRC		+= $(EGUI_PATH)/style
EGUI_CODE_SRC		+= $(EGUI_PATH)/resource
EGUI_CODE_SRC		+= $(EGUI_PATH)/utils
EGUI_CODE_SRC		+= $(EGUI_PATH)/utils/simple_ringbuffer

EGUI_CODE_SRC_FILES += \
    third_party/plutosvg/source/plutosvg.c \
    third_party/plutovg/source/plutovg-blend.c \
    third_party/plutovg/source/plutovg-canvas.c \
    third_party/plutovg/source/plutovg-font.c \
    third_party/plutovg/source/plutovg-ft-math.c \
    third_party/plutovg/source/plutovg-ft-raster.c \
    third_party/plutovg/source/plutovg-ft-stroker.c \
    third_party/plutovg/source/plutovg-matrix.c \
    third_party/plutovg/source/plutovg-paint.c \
    third_party/plutovg/source/plutovg-path.c \
    third_party/plutovg/source/plutovg-rasterize.c \
    third_party/plutovg/source/plutovg-surface.c

EGUI_CODE_INCLUDE	+= third_party/plutosvg/source
EGUI_CODE_INCLUDE	+= third_party/plutovg/include

EGUI_CODE_INCLUDE	+= $(EGUI_PATH)
