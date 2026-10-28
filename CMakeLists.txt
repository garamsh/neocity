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
add_subdirectory(src/ascent/engine)
add_subdirectory(src/ascent/godot)
