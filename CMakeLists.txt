cmake_minimum_required(VERSION 3.25)
project(xml LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# strnlen is POSIX rather than ISO C; glibc declares it only for POSIX.1-2008
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(_POSIX_C_SOURCE=200809L)
endif()

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
        src/os/win32/os_file_directory.c
        src/os/win32/os_time.c)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(os PRIVATE
        src/os/linux/os_diagnostics.c
        src/os/linux/os_file_directory.c
        src/os/linux/os_time.c)
    target_compile_definitions(os PRIVATE _GNU_SOURCE)
    target_link_libraries(os PUBLIC m)
else()
    message(FATAL_ERROR "The OS library supports Windows and Linux")
endif()
target_include_directories(os PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(faceoe_utl STATIC
    src/faceoe_utl/faceoe_utl_math.c
    src/faceoe_utl/faceoe_utl_print.c
    src/faceoe_utl/faceoe_utl_scan.c
    src/faceoe_utl/faceoe_utl_util.c
    src/faceoe_utl/faceoe_utl_xml.c
    include/faceoe_utl/faceoe_utl_defines.h
    include/faceoe_utl/faceoe_utl_math.h
    include/faceoe_utl/faceoe_utl_print.h
    include/faceoe_utl/faceoe_utl_scan.h
    include/faceoe_utl/faceoe_utl_util.h
    include/faceoe_utl/faceoe_utl_xml.h)
target_link_libraries(faceoe_utl PUBLIC os)
target_include_directories(faceoe_utl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(faceoe_gfx STATIC
    src/faceoe_gfx/faceoe_gfx_drawlist.c
    include/faceoe_gfx/faceoe_gfx_drawlist.h)
target_link_libraries(faceoe_gfx PUBLIC os)
target_include_directories(faceoe_gfx PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(arinc661 STATIC
    src/arinc661/arinc661_definition.c
    src/arinc661/arinc661_scene.c
    include/arinc661/arinc661.h
    include/arinc661/arinc661_definition.h
    include/arinc661/arinc661_scene.h)
target_link_libraries(arinc661 PUBLIC faceoe_utl faceoe_gfx)
target_include_directories(arinc661 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(faceoe STATIC
    src/faceoe/faceoe_displaymanagement.c
    include/faceoe/faceoe_displaymanagement.h)
target_link_libraries(faceoe PUBLIC faceoe_utl)
target_include_directories(faceoe PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_executable(unit_tests
    tests/faceoe_utl_tests.c
    tests/arinc661_definition_tests.c
    tests/arinc661_scene_tests.c
    tests/faceoe_gfx_tests.c
    tests/faceoe_displaymanagement_tests.c
    tests/arinc661_update_tests.c
    examples/arinc661_update.c)
target_link_libraries(unit_tests PRIVATE faceoe_utl arinc661 faceoe winhttp bcrypt)

add_executable(arinc661_example examples/arinc661_example.c)
target_link_libraries(arinc661_example PRIVATE arinc661 faceoe)
set_target_properties(arinc661_example PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

if(WIN32)
    set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui-docking")
    add_library(imgui STATIC
        "${IMGUI_DIR}/imgui.cpp"
        "${IMGUI_DIR}/imgui_demo.cpp"
        "${IMGUI_DIR}/imgui_draw.cpp"
        "${IMGUI_DIR}/imgui_tables.cpp"
        "${IMGUI_DIR}/imgui_widgets.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
        "${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp")
    target_compile_features(imgui PUBLIC cxx_std_11)
    target_include_directories(imgui PUBLIC "${IMGUI_DIR}" "${IMGUI_DIR}/backends")

    add_executable(cds_editor
        examples/main.cpp
        examples/arinc661_configuration.cpp
        examples/arinc661_configuration.h
        examples/arinc661_definition_editor.cpp
        examples/arinc661_definition_editor.h
        examples/arinc661_context.cpp
        examples/arinc661_context.h
        examples/arinc661_gfx_imgui.cpp
        examples/arinc661_gfx_imgui.h
        examples/arinc661_help.c
        examples/arinc661_help.h
        examples/arinc661_icons.cpp
        examples/arinc661_icons.h
        examples/arinc661_update.c
        examples/arinc661_update.h
        examples/arinc661_version.h
        examples/cds_editor.rc)
    target_link_libraries(cds_editor PRIVATE faceoe arinc661 imgui opengl32 gdi32 dwmapi ole32 shell32 winhttp bcrypt)
    set_target_properties(cds_editor PROPERTIES OUTPUT_NAME "cds-editor" VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

    add_executable(arinc661_definition_editor_tests tests/arinc661_definition_editor_tests.cpp examples/arinc661_gfx_imgui.cpp examples/arinc661_help.c examples/arinc661_context.cpp examples/arinc661_configuration.cpp examples/arinc661_icons.cpp)
    target_include_directories(arinc661_definition_editor_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/examples")
    target_link_libraries(arinc661_definition_editor_tests PRIVATE arinc661 faceoe imgui)
    add_test(NAME arinc661_definition_editor_tests COMMAND arinc661_definition_editor_tests)

    # GUI tests drive the editor panels through the null backend with synthetic input
    add_executable(arinc661_gui_tests tests/arinc661_gui_tests.cpp examples/arinc661_gfx_imgui.cpp examples/arinc661_help.c examples/arinc661_context.cpp examples/arinc661_configuration.cpp examples/arinc661_icons.cpp)
    target_include_directories(arinc661_gui_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/examples")
    target_link_libraries(arinc661_gui_tests PRIVATE arinc661 faceoe imgui)
    add_test(NAME arinc661_gui_tests COMMAND arinc661_gui_tests "${CMAKE_CURRENT_SOURCE_DIR}/examples")

    # Shell extension: content-sniffed overlay badges and a context menu verb for Explorer
    add_library(cds_shell SHARED shell/cds_shell.c shell/cds_shell.h shell/cds_shell.def shell/cds_shell.rc)
    target_link_libraries(cds_shell PRIVATE shlwapi ole32 shell32 uuid)
    target_compile_definitions(cds_shell PRIVATE UNICODE _UNICODE)
    set_target_properties(cds_shell PROPERTIES OUTPUT_NAME "cds-shell")

    # MSI through the WiX toolset when its command line tool is installed
    find_program(WIX_EXECUTABLE wix)
    if(WIX_EXECUTABLE)
        add_custom_target(installer
            COMMAND "${WIX_EXECUTABLE}" build -arch x64
                -d "BinDir=$<TARGET_FILE_DIR:cds_editor>"
                -d "SourceDir=${CMAKE_CURRENT_SOURCE_DIR}"
                -o "${CMAKE_BINARY_DIR}/cds-editor.msi"
                "${CMAKE_CURRENT_SOURCE_DIR}/installer/cds_editor.wxs"
            DEPENDS cds_editor cds_shell
            COMMENT "Building cds-editor.msi"
            VERBATIM)

        # Setup wizard: a native program that embeds the MSI and drives it through the external UI
        add_executable(cds_setup WIN32 setup/cds_setup.c setup/resource.h setup/cds_setup.rc setup/cds_setup.manifest)
        target_include_directories(cds_setup PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
        target_compile_definitions(cds_setup PRIVATE UNICODE _UNICODE)
        target_link_libraries(cds_setup PRIVATE msi comctl32 uxtheme dwmapi shell32 shlwapi ole32 gdi32 msimg32 advapi32)
        set_source_files_properties(setup/cds_setup.rc PROPERTIES COMPILE_DEFINITIONS "CDS_SETUP_PACKAGE_PATH=\"${CMAKE_BINARY_DIR}/cds-editor.msi\"")
        set_target_properties(cds_setup PROPERTIES OUTPUT_NAME "cds-setup" LINK_FLAGS "/MANIFEST:NO")
        add_dependencies(cds_setup installer)
    endif()
endif()

enable_testing()
add_test(NAME unit_tests COMMAND unit_tests)
set_tests_properties(unit_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests")
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT unit_tests)
