# HelloBasic: uses APP_SUB variable to select sub-application
# Usage: cmake -DAPP=HelloBasic -DAPP_SUB=button ..

# Common source files
file(GLOB HELLO_BASIC_SRC *.c)

# Sub-app source files
if(NOT APP_SUB STREQUAL "")
    file(GLOB APP_SUB_SRC ${APP_SUB}/*.c)
    file(GLOB_RECURSE APP_SUB_RESOURCE_SRC ${APP_SUB}/resource/*.c)
    list(APPEND HELLO_BASIC_SRC ${APP_SUB_SRC} ${APP_SUB_RESOURCE_SRC})

    if(APP_SUB STREQUAL "file_image")
        file(GLOB APP_SUB_MOUNT_ROUTER_SRC ${APP_SUB}/mount_router_template/*.c)
        list(APPEND HELLO_BASIC_SRC ${APP_SUB_MOUNT_ROUTER_SRC})
        list(APPEND HELLO_BASIC_SRC
            ${CMAKE_SOURCE_DIR}/third_party/file_image/decoder_bmp_stream.c
            ${CMAKE_SOURCE_DIR}/third_party/file_image/decoder_stb.c
            ${CMAKE_SOURCE_DIR}/third_party/file_image/decoder_tjpgd_stream.c
            ${CMAKE_SOURCE_DIR}/third_party/file_image/third_party_tjpgd.c
        )
    elseif(APP_SUB STREQUAL "deferred_image")
        include_directories(${CMAKE_SOURCE_DIR}/example/HelloBasic/file_image)
        list(APPEND HELLO_BASIC_SRC
            ${CMAKE_SOURCE_DIR}/example/HelloBasic/file_image/file_io_stdio.c
            ${CMAKE_SOURCE_DIR}/third_party/file_image/decoder_bmp_stream.c
        )
    endif()
endif()

add_library(example_lib ${HELLO_BASIC_SRC})
