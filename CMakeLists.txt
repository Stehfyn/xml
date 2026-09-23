cmake_minimum_required(VERSION 3.25)
project(xml LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(utl STATIC
    src/utl/utl_xml.c
    include/utl/utl_defines.h
    include/utl/utl_xml.h)
target_include_directories(utl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_executable(utl_tests tests/utl_tests.c)
target_link_libraries(utl_tests PRIVATE utl)

enable_testing()
add_test(NAME utl_tests COMMAND utl_tests)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT utl_tests)
