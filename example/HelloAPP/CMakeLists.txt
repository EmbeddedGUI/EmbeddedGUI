# 定义应用程序源文件
set(EXAMPLE_FILES egui_page_base.c
	egui_page_0.c
	egui_page_1.c
	uicode.c)

add_library(example_lib ${EXAMPLE_FILES})
