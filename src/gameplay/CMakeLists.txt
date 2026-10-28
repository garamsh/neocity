# Gameplay links nothing but the C++ standard library: no Godot or
# godot-cpp include directory is reachable from these sources.
add_library(gameplay STATIC src/collision.cpp src/course.cpp src/simulation.cpp)
target_include_directories(gameplay PUBLIC include)
target_compile_options(gameplay PRIVATE ${NEOCITY_WARNINGS})
set_target_properties(
    gameplay
    PROPERTIES POSITION_INDEPENDENT_CODE ON COMPILE_WARNING_AS_ERROR ON
)

include(FetchContent)
# googletest v1.18.0; moved with the other C++ pins listed in Makefile.
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

add_executable(gameplay_test src/collision_test.cpp src/simulation_test.cpp)
target_link_libraries(gameplay_test PRIVATE gameplay GTest::gtest_main)
target_compile_options(gameplay_test PRIVATE ${NEOCITY_WARNINGS})
set_target_properties(gameplay_test PROPERTIES COMPILE_WARNING_AS_ERROR ON)

include(GoogleTest)
gtest_discover_tests(gameplay_test)
