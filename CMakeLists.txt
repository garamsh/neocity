cmake_minimum_required(VERSION 4.4)
project(neocity LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Applied to project targets only; fetched dependencies keep their own flags.
set(NEOCITY_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wdouble-promotion
    -Wshadow
)

enable_testing()
add_subdirectory(src/survival)

include(FetchContent)
# godot-cpp 10.0.0-stable, built against its bundled Godot 4.7 API; moved with
# the other C++ pins listed in Makefile. SOURCE_SUBDIR names no directory, so
# FetchContent_MakeAvailable only downloads it and add_subdirectory adds it below.
FetchContent_Declare(
    godot-cpp
    URL https://github.com/godotengine/godot-cpp/archive/507ed9d840c01a3c5b2a39af8bb4000bfac30bf5.tar.gz
    URL_HASH SHA256=30da4ac295997061a6af81ae531510efd4e6a86e506eb4466edc0898ef3fe048
    SOURCE_SUBDIR none
)
FetchContent_MakeAvailable(godot-cpp)

# godot-cpp's own GODOTCPP_BUILD_PROFILE rewrites the trimmed API file on every
# configure, and the new timestamp makes its binding generator and every godot-cpp
# object run again. Trimming here replaces the file only when its content changes.
find_package(Python3 REQUIRED)
set(NEOCITY_API_FILE "${CMAKE_CURRENT_BINARY_DIR}/extension_api.json")
execute_process(
    COMMAND
        "${Python3_EXECUTABLE}" "${godot-cpp_SOURCE_DIR}/build_profile.py" "${PROJECT_SOURCE_DIR}/build_profile.json"
        "${godot-cpp_SOURCE_DIR}/gdextension/extension_api-4-7.json" "${NEOCITY_API_FILE}.new"
    COMMAND_ERROR_IS_FATAL ANY
)
file(COPY_FILE "${NEOCITY_API_FILE}.new" "${NEOCITY_API_FILE}" ONLY_IF_DIFFERENT)
set(GODOTCPP_CUSTOM_API_FILE "${NEOCITY_API_FILE}" CACHE FILEPATH "" FORCE)
set(GODOTCPP_BUILD_PROFILE "" CACHE FILEPATH "" FORCE)
add_subdirectory("${godot-cpp_SOURCE_DIR}" "${godot-cpp_BINARY_DIR}" SYSTEM)

add_library(
    neocity
    SHARED
    src/audio/sound.cpp
    src/audio/speaker.cpp
    src/hud/menus.cpp
    src/hud/pointer.cpp
    src/hud/survival.cpp
    src/input/menu.cpp
    src/input/survival.cpp
    src/register_types.cpp
    src/rendering/camera.cpp
    src/rendering/drawing.cpp
    src/rendering/feedback.cpp
    src/rendering/survival.cpp
    src/scene.cpp
    src/session/session.cpp
    src/session/sound.cpp
    src/session/survival_run.cpp
    src/session/tips.cpp
)
target_link_libraries(neocity PRIVATE survival godot-cpp)
target_compile_options(neocity PRIVATE ${NEOCITY_WARNINGS})
# One library for editor and game, written to project/bin/ beside
# neocity.gdextension, which loads it.
set_target_properties(
    neocity
    PROPERTIES
        COMPILE_WARNING_AS_ERROR ON
        PREFIX "lib"
        LIBRARY_OUTPUT_DIRECTORY "$<1:${PROJECT_SOURCE_DIR}/project/bin>"
        RUNTIME_OUTPUT_DIRECTORY "$<1:${PROJECT_SOURCE_DIR}/project/bin>"
)
