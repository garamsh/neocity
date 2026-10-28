include(FetchContent)
# godot-cpp 10.0.0-stable, built against its bundled Godot 4.7 API; moved with
# the other C++ pins listed in Makefile.
FetchContent_Declare(
    godot-cpp
    URL https://github.com/godotengine/godot-cpp/archive/507ed9d840c01a3c5b2a39af8bb4000bfac30bf5.tar.gz
    URL_HASH SHA256=30da4ac295997061a6af81ae531510efd4e6a86e506eb4466edc0898ef3fe048
    SYSTEM
)
set(GODOTCPP_DEFAULT_API_VERSION "4.7")
set(GODOTCPP_BUILD_PROFILE "${PROJECT_SOURCE_DIR}/build_profile.json" CACHE FILEPATH "" FORCE)
FetchContent_MakeAvailable(godot-cpp)

add_library(
    neocity_ascent
    SHARED
    src/camera.cpp
    src/drawing.cpp
    src/hud.cpp
    src/input.cpp
    src/register_types.cpp
    src/scene.cpp
)
target_link_libraries(neocity_ascent PRIVATE ascent_engine godot-cpp)
target_compile_options(neocity_ascent PRIVATE ${NEOCITY_WARNINGS})
# One library for editor and game, written to project/bin/ beside
# neocity.gdextension, which loads it.
set_target_properties(
    neocity_ascent
    PROPERTIES
        COMPILE_WARNING_AS_ERROR ON
        PREFIX "lib"
        LIBRARY_OUTPUT_DIRECTORY "$<1:${PROJECT_SOURCE_DIR}/project/bin>"
        RUNTIME_OUTPUT_DIRECTORY "$<1:${PROJECT_SOURCE_DIR}/project/bin>"
)
