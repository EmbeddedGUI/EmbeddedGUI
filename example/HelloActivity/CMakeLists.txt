# 定义应用程序源文件
set(EXAMPLE_FILES egui_activity_test.c
	egui_dialog_test.c
	uicode.c)

add_library(example_lib ${EXAMPLE_FILES})
