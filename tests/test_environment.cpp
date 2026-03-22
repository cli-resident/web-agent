#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <cpr/cpr.h>
#include <filesystem>

// TC-ENV-01: Compiler supports C++17
TEST(EnvironmentTest, TC_ENV_01_Cpp17Support) {
    EXPECT_GE(__cplusplus, 201703L)
        << "Compiler does not support C++17 (__cplusplus=" << __cplusplus << ")";
}

// TC-ENV-02: nlohmann/json is linked — parse simple JSON
TEST(EnvironmentTest, TC_ENV_02_NlohmannJson) {
    nlohmann::json j = nlohmann::json::parse(R"({"key": "value", "num": 42})");
    EXPECT_EQ(j["key"].get<std::string>(), "value");
    EXPECT_EQ(j["num"].get<int>(), 42);
}

// TC-ENV-03: spdlog is linked — logger initialises without exception
TEST(EnvironmentTest, TC_ENV_03_SpdlogInit) {
    ASSERT_NO_THROW({
        auto logger = spdlog::stdout_color_mt("env_test_logger");
        logger->info("TC-ENV-03 spdlog test");
        spdlog::drop("env_test_logger");
    });
}

// TC-ENV-04: cpr is linked — cpr::Session object created without exception
TEST(EnvironmentTest, TC_ENV_04_CprSession) {
    ASSERT_NO_THROW({
        cpr::Session session;
        (void)session;
    });
}

// TC-ENV-05: Working directories can be created via std::filesystem
TEST(EnvironmentTest, TC_ENV_05_FilesystemCreateDir) {
    namespace fs = std::filesystem;
    auto tmp = fs::temp_directory_path() / "wa_env_test_dir";
    ASSERT_NO_THROW(fs::create_directories(tmp));
    EXPECT_TRUE(fs::exists(tmp));
    fs::remove_all(tmp);
}
