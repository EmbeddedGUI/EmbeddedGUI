# HelloBasic: uses APP_SUB variable to select sub-application
# Usage: cmake -DAPP=HelloBasic -DAPP_SUB=button ..

# Common source files
file(GLOB HELLO_BASIC_SRC *.c)

# Sub-app source files
if(NOT APP_SUB STREQUAL "")
    file(GLOB APP_SUB_SRC ${APP_SUB}/*.c)
    file(GLOB_RECURSE APP_SUB_RESOURCE_SRC ${APP_SUB}/resource/*.c)
    list(APPEND HELLO_BASIC_SRC ${APP_SUB_SRC} ${APP_SUB_RESOURCE_SRC})
endif()

add_library(example_lib ${HELLO_BASIC_SRC})
