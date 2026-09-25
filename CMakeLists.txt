cmake_minimum_required(VERSION 3.25)
project(xml LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(os STATIC
    src/os/os_assert.c
    src/os/os_file.c
    src/os/os_malloc.c
    src/os/os_message.c
    include/os/os_assert.h
    include/os/os_diagnostics.h
    include/os/os_file.h
    include/os/os_malloc.h
    include/os/os_message.h
    include/os/os_time.h
    include/os/os_types.h)
if(WIN32)
    target_sources(os PRIVATE
        src/os/win32/os_diagnostics.c
        src/os/win32/os_time.c)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(os PRIVATE
        src/os/linux/os_diagnostics.c
        src/os/linux/os_time.c)
    target_compile_definitions(os PRIVATE _GNU_SOURCE)
    target_link_libraries(os PUBLIC m)
else()
    message(FATAL_ERROR "The OS library supports Windows and Linux")
endif()
target_include_directories(os PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(utl STATIC
    src/utl/utl_scan.c
    src/utl/utl_util.c
    src/utl/utl_xml.c
    include/utl/utl_defines.h
    include/utl/utl_scan.h
    include/utl/utl_util.h
    include/utl/utl_xml.h)
target_link_libraries(utl PUBLIC os)
target_include_directories(utl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(utl_arinc661 STATIC
    src/utl/utl_arinc661.c
    include/utl/utl_arinc661.h)
target_link_libraries(utl_arinc661 PUBLIC utl)
target_include_directories(utl_arinc661 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_executable(utl_tests tests/utl_tests.c)
target_link_libraries(utl_tests PRIVATE utl)

add_executable(utl_arinc661_tests tests/utl_arinc661_tests.c)
target_link_libraries(utl_arinc661_tests PRIVATE utl_arinc661)

add_executable(arinc661_definition examples/arinc661_definition.c)
target_link_libraries(arinc661_definition PRIVATE utl_arinc661)
set_target_properties(arinc661_definition PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

if(WIN32)
    set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui-docking")
    add_executable(arinc661_configuration
        examples/main.cpp
        examples/arinc661_configuration.cpp
        examples/arinc661_configuration.h
        "${IMGUI_DIR}/imgui.cpp"
        "${IMGUI_DIR}/imgui_demo.cpp"
        "${IMGUI_DIR}/imgui_draw.cpp"
        "${IMGUI_DIR}/imgui_tables.cpp"
        "${IMGUI_DIR}/imgui_widgets.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
        "${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp")
    target_compile_features(arinc661_configuration PRIVATE cxx_std_11)
    target_include_directories(arinc661_configuration PRIVATE "${IMGUI_DIR}" "${IMGUI_DIR}/backends")
    target_link_libraries(arinc661_configuration PRIVATE utl_arinc661 opengl32 gdi32 dwmapi)
    set_target_properties(arinc661_configuration PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

add_custom_target(arinc661_definition_xml
    COMMAND $<TARGET_FILE:arinc661_definition>
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS arinc661_definition)

enable_testing()
add_test(NAME utl_tests COMMAND utl_tests)
set_tests_properties(utl_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests")
add_test(NAME utl_arinc661_tests COMMAND utl_arinc661_tests)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT utl_tests)
