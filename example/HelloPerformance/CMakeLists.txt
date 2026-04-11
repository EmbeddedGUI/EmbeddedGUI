# Source files
file(GLOB EXAMPLE_FILES *.c)
file(GLOB_RECURSE RESOURCE_SRC_FILES resource/*.c)
set(FILE_IMAGE_HELPER_FILES
    ${CMAKE_SOURCE_DIR}/example/HelloBasic/file_image/decoder_bmp_stream.c
    ${CMAKE_SOURCE_DIR}/example/HelloBasic/file_image/decoder_registry.c
    ${CMAKE_SOURCE_DIR}/example/HelloBasic/file_image/decoder_stb.c
    ${CMAKE_SOURCE_DIR}/example/HelloBasic/file_image/decoder_tjpgd_stream.c
    ${CMAKE_SOURCE_DIR}/example/HelloBasic/file_image/file_image_stack.c
    ${CMAKE_SOURCE_DIR}/example/HelloBasic/file_image/mount_router_template/file_io_mount_router_template.c
    ${CMAKE_SOURCE_DIR}/example/HelloBasic/file_image/third_party_tjpgd.c
)

add_library(example_lib ${EXAMPLE_FILES} ${RESOURCE_SRC_FILES} ${FILE_IMAGE_HELPER_FILES})
