cmake_minimum_required(VERSION 4.4)
project(pan_dodge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(NEOCITY_WARNINGS -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
    -Wdouble-promotion -Wshadow)

add_library(pan_dodge STATIC src/engine.cpp)
target_include_directories(pan_dodge PUBLIC include)
target_compile_options(pan_dodge PRIVATE ${NEOCITY_WARNINGS})
set_target_properties(pan_dodge PROPERTIES POSITION_INDEPENDENT_CODE ON COMPILE_WARNING_AS_ERROR ON)

include(FetchContent)
# googletest v1.18.0; @sauce-git moves this with the pins in Makefile.
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/063de7e9578f82b369302001269680b4b1553359.tar.gz
    URL_HASH SHA256=623fe05d283020f37a579a8985ba338091ac7229da62b5591ff40acdf05b95e2
    SYSTEM
    EXCLUDE_FROM_ALL
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
enable_testing()
add_executable(pan_dodge_test src/engine_test.cpp)
target_link_libraries(pan_dodge_test PRIVATE pan_dodge GTest::gtest_main)
target_compile_options(pan_dodge_test PRIVATE ${NEOCITY_WARNINGS})
set_target_properties(pan_dodge_test PROPERTIES COMPILE_WARNING_AS_ERROR ON)
include(GoogleTest)
gtest_discover_tests(pan_dodge_test)
