add_executable(wa_tests
    test_config.cpp
    test_environment.cpp
    test_repository.cpp
)
target_link_libraries(wa_tests PRIVATE
    agent_lib
    GTest::gtest
    GTest::gtest_main
)
target_compile_definitions(wa_tests PRIVATE
    PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)
include(GoogleTest)
gtest_discover_tests(wa_tests)
