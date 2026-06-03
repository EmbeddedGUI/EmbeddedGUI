cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME stm32f407zet6)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Add EmbeddedGUI sources
add_subdirectory(EmbeddedGUI)
target_include_directories(EmbeddedGUI PUBLIC 
    ${CMAKE_SOURCE_DIR}/egui_port
)
# Add example sources
# add by  glob
# file(GLOB_RECURSE EXAMPLE_SRC_FILES 
#     ${CMAKE_SOURCE_DIR}/EmbeddedGUI/example/HelloChart/*.c
# )
# set (EXAMPLE_INCLUDE_DIRS 
#     ${CMAKE_SOURCE_DIR}/EmbeddedGUI/example/HelloChart
# )
# add by subdirectory
add_subdirectory(EmbeddedGUI/example/HelloEasyPage)
target_link_libraries(example_lib PRIVATE
    EmbeddedGUI
)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/EmbeddedGUI/example/HelloEasyPage
)


# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here

    # egui driver files
    ${CMAKE_SOURCE_DIR}/EmbeddedGUI/driver/lcd/egui_lcd_st7789.c
    ${CMAKE_SOURCE_DIR}/EmbeddedGUI/driver/touch/egui_touch_ft6336.c
    ${CMAKE_SOURCE_DIR}/EmbeddedGUI/driver/bus/egui_panel_io_spi.c
    ${CMAKE_SOURCE_DIR}/EmbeddedGUI/driver/bus/egui_panel_io_i2c.c
    ${CMAKE_SOURCE_DIR}/EmbeddedGUI/driver/lcd/egui_lcd.c
    ${CMAKE_SOURCE_DIR}/EmbeddedGUI/driver/touch/egui_touch.c
    # port files
    ${CMAKE_SOURCE_DIR}/egui_port/port_main.c
    ${CMAKE_SOURCE_DIR}/egui_port/egui_port_mcu.c
    ${CMAKE_SOURCE_DIR}/egui_port/egui_hal_stm32.c
    ${CMAKE_SOURCE_DIR}/egui_port/SoftIIC/SoftIIC.c
    # example file
    # add glob
    # ${EXAMPLE_SRC_FILES}
    # add manual
    # ${CMAKE_SOURCE_DIR}/EmbeddedGUI/example/HelloSimple/uicode_disp0.c
    # ${CMAKE_SOURCE_DIR}/EmbeddedGUI/example/HelloSimple/resource/font/egui_res_font_hello_simple_14_4.c
    # ${CMAKE_SOURCE_DIR}/EmbeddedGUI/example/HelloSimple/resource/app_egui_resource_generate.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    # port files
    ${CMAKE_SOURCE_DIR}/egui_port
    ${CMAKE_SOURCE_DIR}/egui_port/SoftIIC
    ${CMAKE_SOURCE_DIR}/EmbeddedGUI/driver/touch
    ${CMAKE_SOURCE_DIR}/EmbeddedGUI/driver/bus
    ${CMAKE_SOURCE_DIR}/EmbeddedGUI/driver/lcd

    # example file
    # add glob
    # ${EXAMPLE_INCLUDE_DIRS}
    # add manual
    # ${CMAKE_SOURCE_DIR}/EmbeddedGUI/example/HelloChart

)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    EGUI_DRIVER_TOUCH_FT6336_ENABLE=1
    EGUI_DRIVER_LCD_ST7789_ENABLE=1
    EGUI_DRIVER_PANEL_IO_SPI_ENABLE=1
    EGUI_DRIVER_PANEL_IO_I2C_ENABLE=1
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
    EmbeddedGUI
    example_lib
)
