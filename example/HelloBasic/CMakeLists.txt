# 定义子功能选项
option(ENABLE_ANIM "Enable anim functionality" OFF)
option(ENABLE_BUTTON "Enable button functionality" OFF)
option(ENABLE_BUTTON_IMAGE "Enable button image functionality" OFF)
option(ENABLE_IMAGE "Enable image functionality" OFF)
option(ENABLE_LABEL "Enable label functionality" OFF)
option(ENABLE_LINEARLAYOUT "Enable linearlayout functionality" OFF)
option(ENABLE_MASK "Enable mask functionality" OFF)
option(ENABLE_PROGRESS_BAR "Enable progress_bar functionality" OFF)
option(ENABLE_SCROLL "Enable scroll functionality" OFF)
option(ENABLE_SWITCH "Enable switch functionality" OFF)
option(ENABLE_VIEWPAGE "Enable viewpage functionality" OFF)
option(ENABLE_VIEWPAGE_CACHE "Enable viewpage_cache functionality" ON)

set(HELLO_BASIC_SRC uicode.c)

if(ENABLE_ANIM)
    list(APPEND HELLO_BASIC_SRC anim/test.c)
endif()

if(ENABLE_BUTTON)
	list(APPEND HELLO_BASIC_SRC button/test.c)
endif()

if(ENABLE_BUTTON_IMAGE)
	list(APPEND HELLO_BASIC_SRC button_img/test.c)
	list(APPEND HELLO_BASIC_SRC button_img/resource/egui_res_image_star_rgb565_8.c)
	list(APPEND HELLO_BASIC_SRC button_img/resource/egui_res_image_test_rgb565_8.c)
endif()

if(ENABLE_IMAGE)
	list(APPEND HELLO_BASIC_SRC image/test.c)
	list(APPEND HELLO_BASIC_SRC image/resource/egui_res_image_star_rgb565_8.c)
	list(APPEND HELLO_BASIC_SRC image/resource/egui_res_image_test_rgb565_8.c)
endif()

if(ENABLE_LABEL)
	list(APPEND HELLO_BASIC_SRC label/test.c)
endif()

if(ENABLE_LINEARLAYOUT)
	list(APPEND HELLO_BASIC_SRC linearlayout/test.c)
endif()

if(ENABLE_MASK)
	list(APPEND HELLO_BASIC_SRC mask/test.c)
	list(APPEND HELLO_BASIC_SRC mask/egui_view_test_mask.c)
	list(APPEND HELLO_BASIC_SRC mask/resource/egui_res_image_star_rgb565_8.c)
	list(APPEND HELLO_BASIC_SRC mask/resource/egui_res_image_test_rgb565_8.c)
endif()

if(ENABLE_PROGRESS_BAR)
	list(APPEND HELLO_BASIC_SRC progress_bar/test.c)
endif()

if(ENABLE_SCROLL)
	list(APPEND HELLO_BASIC_SRC scroll/test.c)
endif()

if(ENABLE_SWITCH)
	list(APPEND HELLO_BASIC_SRC switch/test.c)
endif()

if(ENABLE_VIEWPAGE)
	list(APPEND HELLO_BASIC_SRC viewpage/test.c)
endif()

if(ENABLE_VIEWPAGE_CACHE)
	list(APPEND HELLO_BASIC_SRC viewpage_cache/test.c)
	list(APPEND HELLO_BASIC_SRC viewpage_cache/egui_view_page_test.c)
endif()

add_library(example_lib ${HELLO_BASIC_SRC})
