# Headless test port (no SDL2)
set(PC_TEST_SRC_FILES
    ${CMAKE_SOURCE_DIR}/porting/pc_test/main.c
    ${CMAKE_SOURCE_DIR}/porting/pc_test/port_api.c
)

# Include directory
include_directories(${CMAKE_SOURCE_DIR}/porting/pc_test)

# Executable
add_executable(${APP} ${PC_TEST_SRC_FILES})
target_link_libraries(${APP} example_lib src_lib)

# Platform-specific linking (no SDL2, only math lib)
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    # No extra libs needed on Windows
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_link_libraries(${APP} m)
else()
    target_link_libraries(${APP} m)
endif()
