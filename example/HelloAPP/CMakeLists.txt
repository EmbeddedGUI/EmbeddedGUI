# Source files
file(GLOB EXAMPLE_FILES *.c)
file(GLOB_RECURSE RESOURCE_SRC_FILES resource/*.c)

# Additional include directories for resources
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/resource/font)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/resource/img)

add_library(example_lib ${EXAMPLE_FILES} ${RESOURCE_SRC_FILES})
