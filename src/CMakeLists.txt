# Source files
file(GLOB_RECURSE ANIM_SRC_FILES anim/*.c)
file(GLOB_RECURSE APP_SRC_FILES app/*.c)
file(GLOB_RECURSE BACKGROUND_SRC_FILES background/*.c)
file(GLOB_RECURSE CANVAS_SRC_FILES canvas/*.c)
file(GLOB_RECURSE CORE_SRC_FILES core/*.c)
file(GLOB_RECURSE FONT_SRC_FILES font/*.c)
file(GLOB_RECURSE IMAGE_SRC_FILES image/*.c)
file(GLOB_RECURSE MASK_SRC_FILES mask/*.c)
file(GLOB_RECURSE RESOURCE_SRC_FILES resource/*.c)
file(GLOB_RECURSE SHADOW_SRC_FILES shadow/*.c)
file(GLOB_RECURSE STYLE_SRC_FILES style/*.c)
file(GLOB_RECURSE UTILS_SRC_FILES utils/*.c)
file(GLOB_RECURSE WIDGET_SRC_FILES widget/*.c)

set(THIRD_PARTY_SRC_FILES
    ${CMAKE_SOURCE_DIR}/third_party/plutosvg/source/plutosvg.c
    ${CMAKE_SOURCE_DIR}/third_party/plutovg/source/plutovg-blend.c
    ${CMAKE_SOURCE_DIR}/third_party/plutovg/source/plutovg-canvas.c
    ${CMAKE_SOURCE_DIR}/third_party/plutovg/source/plutovg-font.c
    ${CMAKE_SOURCE_DIR}/third_party/plutovg/source/plutovg-ft-math.c
    ${CMAKE_SOURCE_DIR}/third_party/plutovg/source/plutovg-ft-raster.c
    ${CMAKE_SOURCE_DIR}/third_party/plutovg/source/plutovg-ft-stroker.c
    ${CMAKE_SOURCE_DIR}/third_party/plutovg/source/plutovg-matrix.c
    ${CMAKE_SOURCE_DIR}/third_party/plutovg/source/plutovg-paint.c
    ${CMAKE_SOURCE_DIR}/third_party/plutovg/source/plutovg-path.c
    ${CMAKE_SOURCE_DIR}/third_party/plutovg/source/plutovg-rasterize.c
    ${CMAKE_SOURCE_DIR}/third_party/plutovg/source/plutovg-surface.c)

# Test sources (only for HelloUnitTest)
set(TEST_SRC_FILES "")
if(APP STREQUAL "HelloUnitTest")
    file(GLOB TEST_SRC_FILES test/*.c)
endif()

# Library
add_library(src_lib
    ${ANIM_SRC_FILES}
    ${APP_SRC_FILES}
    ${BACKGROUND_SRC_FILES}
    ${CANVAS_SRC_FILES}
    ${CORE_SRC_FILES}
    ${FONT_SRC_FILES}
    ${IMAGE_SRC_FILES}
    ${MASK_SRC_FILES}
    ${RESOURCE_SRC_FILES}
    ${SHADOW_SRC_FILES}
    ${STYLE_SRC_FILES}
    ${UTILS_SRC_FILES}
    ${WIDGET_SRC_FILES}
    ${THIRD_PARTY_SRC_FILES}
    ${TEST_SRC_FILES})

