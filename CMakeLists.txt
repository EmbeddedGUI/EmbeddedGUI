# CMake minimum version and project definition
cmake_minimum_required(VERSION 3.10)
project(EGUI_Project C)

# C standard
set(CMAKE_C_STANDARD 99)

# Application selection (same variables as Makefile)
set(APP "HelloSimple" CACHE STRING "Application to build (e.g. HelloSimple, HelloBasic, HelloVirtual, HelloActivity, ...)")
set(APP_SUB "" CACHE STRING "Sub-application for HelloBasic/HelloVirtual (e.g. button, virtual_stage_basic, slider, ...)")
set(PORT "pc" CACHE STRING "Target port (pc, pc_test)")

# Macro definitions
add_compile_definitions(EGUI_APP="${APP}")
if(PORT STREQUAL "pc" OR PORT STREQUAL "designer")
    add_compile_definitions(EGUI_PORT=EGUI_PORT_TYPE_PC)
elseif(PORT STREQUAL "pc_test")
    add_compile_definitions(EGUI_PORT=EGUI_PORT_TYPE_PC)
elseif(PORT STREQUAL "qemu")
    add_compile_definitions(EGUI_PORT=EGUI_PORT_TYPE_QEMU)
else()
    add_compile_definitions(EGUI_PORT=EGUI_PORT_TYPE_MCU)
endif()

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

string(CONCAT _egui_all_cflags
    " " "${CMAKE_C_FLAGS}"
    " " "${CMAKE_C_FLAGS_DEBUG}"
    " " "${CMAKE_C_FLAGS_RELEASE}"
    " " "${CMAKE_C_FLAGS_RELWITHDEBINFO}"
    " " "${CMAKE_C_FLAGS_MINSIZEREL}"
)

if(EXISTS "${CMAKE_SOURCE_DIR}/example/${APP}" AND NOT _egui_all_cflags MATCHES "EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING")
    add_compile_definitions(EGUI_CONFIG_FUNCTION_SUPPORT_MARGIN_PADDING=1)
endif()

# Global include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/example/${APP})

# For apps with sub-apps, add sub-app include paths
if((APP STREQUAL "HelloBasic" OR APP STREQUAL "HelloVirtual" OR APP STREQUAL "HelloCustomWidgets" OR APP STREQUAL "HelloSizeAnalysis") AND NOT APP_SUB STREQUAL "")
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
