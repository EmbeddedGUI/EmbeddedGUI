# driver/build.mk
# HAL Driver Layer - Bus IO and Device Drivers

EGUI_CODE_INCLUDE += \
    driver/bus \
    driver/lcd \
    driver/touch

EGUI_CODE_SRC += \
    driver/bus \
    driver/lcd \
    driver/touch
