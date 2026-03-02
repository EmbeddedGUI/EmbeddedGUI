# Port source files
set(PC_SRC_FILES
    ${CMAKE_SOURCE_DIR}/porting/pc/main.c
    ${CMAKE_SOURCE_DIR}/porting/pc/egui_port_pc.c
    ${CMAKE_SOURCE_DIR}/porting/pc/sdl_port.c
)

# Platform-specific SDL2 configuration
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    message(STATUS "Building on Windows")
    # Detect 32/64-bit
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SDL2_BITS "64")
    else()
        set(SDL2_BITS "32")
    endif()
    include_directories(${CMAKE_SOURCE_DIR}/porting/pc/sdl2/${SDL2_BITS}/include)
    link_directories(${CMAKE_SOURCE_DIR}/porting/pc/sdl2/${SDL2_BITS}/lib)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    message(STATUS "Building on Linux")
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    message(STATUS "Building on Apple platform (MacOS or iOS)")
    include_directories(/usr/local/include/)
    link_directories(/usr/local/lib/)
else()
    message(STATUS "Building on another platform")
endif()

# Executable
add_executable(${APP} ${PC_SRC_FILES})
target_link_libraries(${APP} example_lib src_lib)

# Platform-specific linking
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_link_libraries(${APP} SDL2main SDL2 pthread)
    # Copy SDL2.dll to output
    add_custom_command(TARGET ${APP} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/porting/pc/sdl2/${SDL2_BITS}/bin/SDL2.dll"
            "$<TARGET_FILE_DIR:${APP}>"
        COMMENT "Copying SDL2.dll to output directory"
    )
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    target_link_libraries(${APP} SDL2 pthread)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_link_libraries(${APP} SDL2 SDL2main pthread)
endif()
