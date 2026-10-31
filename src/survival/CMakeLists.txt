add_library(survival STATIC src/simulation.cpp)
target_include_directories(survival PUBLIC include)
target_compile_options(survival PRIVATE ${NEOCITY_WARNINGS})
set_target_properties(
    survival
    PROPERTIES POSITION_INDEPENDENT_CODE ON COMPILE_WARNING_AS_ERROR ON
)

add_executable(survival_test src/simulation_test.cpp)
target_link_libraries(survival_test PRIVATE survival GTest::gtest_main)
target_compile_options(survival_test PRIVATE ${NEOCITY_WARNINGS})
set_target_properties(survival_test PROPERTIES COMPILE_WARNING_AS_ERROR ON)

include(GoogleTest)
gtest_discover_tests(survival_test)
