# CMake minimum version and project definition
cmake_minimum_required(VERSION 3.10)
project(EGUI_Project C)

# C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Application selection (same variables as Makefile)
set(APP "HelloSimple" CACHE STRING "Application to build (e.g. HelloSimple, HelloBasic, HelloVirtual, HelloActivity, ...)")
set(APP_SUB "" CACHE STRING "Sub-application for HelloBasic/HelloVirtual/HelloGame (e.g. button, virtual_stage_basic, snake, ...)")
set(PORT "pc" CACHE STRING "Target port (pc, pc_test)")
set(EGUI_USER_CFLAGS_STD "" CACHE STRING "Optional C language standard flag for compatibility checks")
set(EGUI_USER_CFLAGS "" CACHE STRING "Additional compiler flags for compatibility checks")

# Macro definitions
add_compile_definitions(EGUI_APP="${APP}")
if(PORT STREQUAL "pc" OR PORT STREQUAL "designer")
    add_compile_definitions(EGUI_PORT=EGUI_PORT_TYPE_PC)
    if(MSVC)
        add_compile_options(/W3 /utf-8)
    endif()
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(
            -Wall
            -Wextra
            -Werror=implicit-function-declaration
            -Werror=incompatible-pointer-types
            -Werror=return-type
        )
    endif()
elseif(PORT STREQUAL "pc_test")
    add_compile_definitions(EGUI_PORT=EGUI_PORT_TYPE_PC)
elseif(PORT STREQUAL "qemu")
    add_compile_definitions(EGUI_PORT=EGUI_PORT_TYPE_QEMU)
else()
    add_compile_definitions(EGUI_PORT=EGUI_PORT_TYPE_MCU)
endif()

if(EGUI_USER_CFLAGS_STD)
    separate_arguments(EGUI_USER_CFLAGS_STD_LIST NATIVE_COMMAND "${EGUI_USER_CFLAGS_STD}")
    add_compile_options(${EGUI_USER_CFLAGS_STD_LIST})
endif()

separate_arguments(EGUI_USER_CFLAGS_LIST NATIVE_COMMAND "${EGUI_USER_CFLAGS}")
if(EGUI_USER_CFLAGS_LIST)
    add_compile_options(${EGUI_USER_CFLAGS_LIST})
endif()

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

# Global include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/driver/bus)
include_directories(${CMAKE_SOURCE_DIR}/driver/lcd)
include_directories(${CMAKE_SOURCE_DIR}/driver/touch)
include_directories(${CMAKE_SOURCE_DIR}/example/${APP})
include_directories(${CMAKE_SOURCE_DIR}/porting/${PORT})
include_directories(${CMAKE_SOURCE_DIR}/third_party/file_image)
include_directories(${CMAKE_SOURCE_DIR}/third_party/plutosvg/source)
include_directories(${CMAKE_SOURCE_DIR}/third_party/plutovg/include)

if((PORT STREQUAL "pc" OR PORT STREQUAL "designer") AND CMAKE_SYSTEM_NAME MATCHES "Windows")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SDL2_BITS "64")
    else()
        set(SDL2_BITS "32")
    endif()
    include_directories(${CMAKE_SOURCE_DIR}/porting/pc/sdl2/${SDL2_BITS}/include)
endif()

# For apps with sub-apps, add sub-app include paths
if((APP STREQUAL "HelloBasic" OR APP STREQUAL "HelloVirtual" OR APP STREQUAL "HelloSizeAnalysis" OR APP STREQUAL "HelloGame") AND NOT APP_SUB STREQUAL "")
    include_directories(${CMAKE_SOURCE_DIR}/example/${APP}/${APP_SUB})
    if(EXISTS "${CMAKE_SOURCE_DIR}/example/${APP}/${APP_SUB}/resource")
        include_directories(${CMAKE_SOURCE_DIR}/example/${APP}/${APP_SUB}/resource)
    endif()
else()
    if(EXISTS "${CMAKE_SOURCE_DIR}/example/${APP}/resource")
        include_directories(${CMAKE_SOURCE_DIR}/example/${APP}/resource)
    endif()
endif()

# Add subdirectories
add_subdirectory(src)
add_subdirectory(example/${APP})
add_subdirectory(porting/${PORT})
