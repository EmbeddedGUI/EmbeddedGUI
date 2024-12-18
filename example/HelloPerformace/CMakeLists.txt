# 定义应用程序源文件
file(GLOB_RECURSE RESOURCE_SRC_FILES resource/*.c)
set(EXAMPLE_FILES egui_view_test_performance.c
	uicode.c)

add_library(example_lib ${EXAMPLE_FILES} ${RESOURCE_SRC_FILES})
