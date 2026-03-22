cmake_minimum_required(VERSION 3.16)
project(web_agent VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(WA_BUILD_TESTS "Build tests" ON)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include(cmake/Dependencies.cmake)

# Static library (needed for tests in LR #2+)
add_library(agent_lib STATIC
    src/config.cpp
    src/logger.cpp
    src/http_client.cpp
    src/agent.cpp
)
target_include_directories(agent_lib PUBLIC include)
target_link_libraries(agent_lib PUBLIC
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    cpr::cpr
)

# Executable
add_executable(web_agent src/main.cpp)
target_link_libraries(web_agent PRIVATE agent_lib)

if(WA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
