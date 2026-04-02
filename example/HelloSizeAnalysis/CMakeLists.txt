# HelloSizeAnalysis: uses APP_SUB variable to select sub-application

file(GLOB HELLO_SIZE_ANALYSIS_SRC *.c)

if(NOT APP_SUB STREQUAL "")
    file(GLOB APP_SUB_SRC ${APP_SUB}/*.c)
    file(GLOB_RECURSE APP_SUB_RESOURCE_SRC ${APP_SUB}/resource/*.c)
    list(APPEND HELLO_SIZE_ANALYSIS_SRC ${APP_SUB_SRC} ${APP_SUB_RESOURCE_SRC})
endif()

add_library(example_lib ${HELLO_SIZE_ANALYSIS_SRC})
