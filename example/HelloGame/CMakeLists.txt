# HelloGame: uses APP_SUB variable to select sub-application.

file(GLOB HELLO_GAME_SRC *.c)

if(NOT APP_SUB STREQUAL "")
    file(GLOB APP_SUB_SRC ${APP_SUB}/*.c)
    list(APPEND HELLO_GAME_SRC ${APP_SUB_SRC})
endif()

add_library(example_lib ${HELLO_GAME_SRC})

