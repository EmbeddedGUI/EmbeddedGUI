# Source files
file(GLOB EXAMPLE_FILES *.c)
file(GLOB TEST_SRC_FILES test/*.c)

# Additional include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/test)
include_directories(${CMAKE_SOURCE_DIR}/src/test)

add_library(example_lib ${EXAMPLE_FILES} ${TEST_SRC_FILES})
