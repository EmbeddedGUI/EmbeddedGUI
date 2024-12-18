# 定义端口源文件
set(PC_SRC_FILES
    ${CMAKE_SOURCE_DIR}/porting/pc/main.c
    ${CMAKE_SOURCE_DIR}/porting/pc/api_pc.c
    ${CMAKE_SOURCE_DIR}/porting/pc/sdl_port.c
)

# 定义端口库
add_executable(${APP} ${PC_SRC_FILES})
target_link_libraries(${APP} example_lib src_lib SDL2 pthread)

# 定义生成资源文件的命令
# add_custom_command(
#     OUTPUT ${CMAKE_BINARY_DIR}/output/app_egui_resource_merge.bin
#     COMMAND python ${CMAKE_SOURCE_DIR}/scripts/tools/app_resource_generate.py
#             -r ${CMAKE_SOURCE_DIR}/example/${APP}/resource
#             -o ${CMAKE_BINARY_DIR}/output
#     DEPENDS ${CMAKE_SOURCE_DIR}/example/${APP}/resource/*
#     COMMENT "Generating resource file"
# )

# # 添加资源目标
# add_custom_target(resource ALL DEPENDS ${CMAKE_BINARY_DIR}/output/app_egui_resource_merge.bin)
