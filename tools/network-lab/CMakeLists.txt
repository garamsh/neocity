cmake_minimum_required(VERSION 4.4)
project(neocity_network_lab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

set(LAB_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wdouble-promotion
    -Wshadow
)

# Builds the real rules without src/survival/CMakeLists.txt, which needs the
# root build's GoogleTest target.
set(SURVIVAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../src/survival")
file(GLOB SURVIVAL_SOURCES CONFIGURE_DEPENDS "${SURVIVAL_DIR}/src/*.cpp")
list(FILTER SURVIVAL_SOURCES EXCLUDE REGEX "_test\\.cpp$")
add_library(survival STATIC ${SURVIVAL_SOURCES})
target_include_directories(survival PUBLIC "${SURVIVAL_DIR}/include")

add_library(lab_core STATIC src/bot.cpp src/room.cpp src/wire.cpp)
target_link_libraries(lab_core PUBLIC survival)
target_compile_options(lab_core PRIVATE ${LAB_WARNINGS})
set_target_properties(lab_core PROPERTIES COMPILE_WARNING_AS_ERROR ON)

add_executable(
    neocity-22-lab
    src/authority.cpp
    src/client.cpp
    src/main.cpp
    src/net.cpp
    src/relay.cpp
)
target_link_libraries(neocity-22-lab PRIVATE lab_core)
target_compile_options(neocity-22-lab PRIVATE ${LAB_WARNINGS})
set_target_properties(neocity-22-lab PROPERTIES COMPILE_WARNING_AS_ERROR ON)

include(FetchContent)
# googletest v1.18.0, the commit src/gameplay/CMakeLists.txt pins; @sauce-git
# moves it with that pin.
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/063de7e9578f82b369302001269680b4b1553359.tar.gz
    URL_HASH SHA256=623fe05d283020f37a579a8985ba338091ac7229da62b5591ff40acdf05b95e2
    SYSTEM
    EXCLUDE_FROM_ALL
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(lab_test src/room_test.cpp src/wire_test.cpp)
target_link_libraries(lab_test PRIVATE lab_core GTest::gtest_main)
target_compile_options(lab_test PRIVATE ${LAB_WARNINGS})
set_target_properties(lab_test PROPERTIES COMPILE_WARNING_AS_ERROR ON)
include(GoogleTest)
gtest_discover_tests(lab_test)
