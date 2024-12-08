# 定义应用程序源文件
file(GLOB_RECURSE FONT_SRC_FILES resource/font/*.c)
file(GLOB_RECURSE IMG_SRC_FILES resource/img/*.c)
set(EXAMPLE_FILES uicode.c resource/app_egui_resource_generate.c)

add_library(example_lib ${EXAMPLE_FILES} ${FONT_SRC_FILES} ${IMG_SRC_FILES})
