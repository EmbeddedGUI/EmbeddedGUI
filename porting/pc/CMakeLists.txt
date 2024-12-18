# 定义端口源文件
set(PC_SRC_FILES
    ${CMAKE_SOURCE_DIR}/porting/pc/main.c
    ${CMAKE_SOURCE_DIR}/porting/pc/api_pc.c
    ${CMAKE_SOURCE_DIR}/porting/pc/sdl_port.c
)


if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    message(STATUS "Building on Linux")
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    message(STATUS "Building on Apple platform (MacOS or iOS)")
    include_directories(/usr/local/include/)
    link_directories(/usr/local/lib/)
else()
    message(STATUS "Building on another platform")
endif()


# 定义端口库
add_executable(${APP} ${PC_SRC_FILES})
target_link_libraries(${APP} example_lib src_lib)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_libraries(${APP} SDL2 pthread)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_link_libraries(${APP} SDL2 SDL2main pthread)
else()
endif()



# add_compile_definitions(SDL_DISABLE_ARM_NEON_H)

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
