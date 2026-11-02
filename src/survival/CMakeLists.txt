add_library(survival STATIC src/simulation.cpp src/world.cpp src/exploration.cpp src/workshop.cpp src/companion.cpp src/content.cpp src/escape.cpp)
target_include_directories(survival PUBLIC include)
target_compile_options(survival PRIVATE ${NEOCITY_WARNINGS})
set_target_properties(
    survival
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

add_executable(survival_test src/simulation_test.cpp src/world_test.cpp src/exploration_test.cpp src/workshop_test.cpp src/companion_test.cpp src/content_test.cpp src/escape_test.cpp)
target_link_libraries(survival_test PRIVATE survival GTest::gtest_main)
target_compile_options(survival_test PRIVATE ${NEOCITY_WARNINGS})
set_target_properties(survival_test PROPERTIES COMPILE_WARNING_AS_ERROR ON)

include(GoogleTest)
gtest_discover_tests(survival_test)
