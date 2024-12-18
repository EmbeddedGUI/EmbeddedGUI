# CMake 最低版本和项目定义
cmake_minimum_required(VERSION 3.10)
project(EGUI_Project C)

# 设置 C 标准
set(CMAKE_C_STANDARD 99)

# set(CMAKE_CXX_COMPILER "g++")
# set(CMAKE_C_COMPILER "gcc")

option(BUILD_HELLO_ACTIVITY "Build HelloActivity example" OFF)
option(BUILD_HELLO_BASIC "Build HelloBasic example" ON)
option(BUILD_HELLO_PERFORMANCE "Build HelloPerformance example" OFF)
option(BUILD_HELLO_PFB "Build HelloPFB example" OFF)
option(BUILD_HELLO_RESOURCE_MANAGER "Build HelloResourceManager example" OFF)
option(BUILD_HELLO_SIMPLE "Build HelloSimple example" OFF)
option(BUILD_HELLO_TEST "Build HelloTest example" OFF)
option(BUILD_HELLO_VIEWPAGE_SCROLL "Build HelloViewPageAndScroll example" OFF)

if(BUILD_HELLO_ACTIVITY)
    set(APP HelloActivity CACHE STRING "The application to build")
endif()

if(BUILD_HELLO_BASIC)
    set(APP HelloBasic CACHE STRING "The application to build")
endif()

if(BUILD_HELLO_PERFORMANCE)
    set(APP HelloPerformace CACHE STRING "The application to build")
endif()

if(BUILD_HELLO_PFB)
    set(APP HelloPFB CACHE STRING "The application to build")
endif()

if(BUILD_HELLO_RESOURCE_MANAGER)
    set(APP HelloResourceManager CACHE STRING "The application to build")
endif()

if(BUILD_HELLO_SIMPLE)
    set(APP HelloSimple CACHE STRING "The application to build")
endif()

if(BUILD_HELLO_TEST)
    set(APP HelloTest CACHE STRING "The application to build")
endif()

if(BUILD_HELLO_VIEWPAGE_SCROLL)
    set(APP HelloViewPageAndScroll CACHE STRING "The application to build")
endif()

set(PORT pc CACHE STRING "The target port")

# 添加宏定义
add_compile_definitions(EGUI_APP=\"${APP}\")
if(${PORT} STREQUAL "pc")
    add_compile_definitions(EGUI_PORT=EGUI_PORT_TYPE_PC)
else()
    add_compile_definitions(EGUI_PORT=EGUI_PORT_TYPE_MCU)
endif()

# 设置全局输出路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output)

# 全局包含目录
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/example/${APP})
include_directories(${CMAKE_SOURCE_DIR}/example/${APP}/resource)

# 添加子目录
add_subdirectory(src)
add_subdirectory(example/${APP})
add_subdirectory(porting/${PORT})
