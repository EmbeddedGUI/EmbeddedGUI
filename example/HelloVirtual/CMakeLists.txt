# HelloVirtual: uses APP_SUB variable to select sub-application
# Usage: cmake -DAPP=HelloVirtual -DAPP_SUB=virtual_stage_basic ..

# Common source files
file(GLOB HELLO_VIRTUAL_SRC *.c)

# Sub-app source files
if(NOT APP_SUB STREQUAL "")
    file(GLOB APP_SUB_SRC ${APP_SUB}/*.c)
    file(GLOB_RECURSE APP_SUB_RESOURCE_SRC ${APP_SUB}/resource/*.c)
    list(APPEND HELLO_VIRTUAL_SRC ${APP_SUB_SRC} ${APP_SUB_RESOURCE_SRC})
endif()

add_library(example_lib ${HELLO_VIRTUAL_SRC})
