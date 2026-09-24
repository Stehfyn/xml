cmake_minimum_required(VERSION 3.25)
project(xml LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(os STATIC
    src/os/os_assert.c
    src/os/os_malloc.c
    src/os/os_message.c
    include/os/os_assert.h
    include/os/os_diagnostics.h
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
    src/utl/utl_xml.c
    src/utl/utl_xml_document.c
    src/utl/utl_xml_node.c
    include/utl/utl_defines.h
    include/utl/utl_xml.h
    include/utl/utl_xml_document.h
    include/utl/utl_xml_node.h)
target_link_libraries(utl PUBLIC os)
target_include_directories(utl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_executable(utl_tests tests/utl_tests.c)
target_link_libraries(utl_tests PRIVATE utl)

add_executable(arinc661_definition examples/arinc661_definition.c)
target_link_libraries(arinc661_definition PRIVATE utl)
set_target_properties(arinc661_definition PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
add_custom_target(arinc661_definition_xml
    COMMAND $<TARGET_FILE:arinc661_definition>
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS arinc661_definition)

enable_testing()
add_test(NAME utl_tests COMMAND utl_tests)
set_tests_properties(utl_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests")
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT utl_tests)
