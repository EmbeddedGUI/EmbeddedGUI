# Source files
file(GLOB EXAMPLE_FILES *.c)
file(GLOB_RECURSE RESOURCE_SRC_FILES resource/*.c)

add_library(example_lib ${EXAMPLE_FILES} ${RESOURCE_SRC_FILES})
