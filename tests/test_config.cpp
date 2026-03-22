#include <gtest/gtest.h>
#include "config.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

// Helper: write a JSON file to a temp path
static std::string writeTempJson(const std::string& content) {
    auto path = fs::temp_directory_path() / "wa_test_config.json";
    std::ofstream f(path);
    f << content;
    return path.string();
}

// TC-CFG-01: Load valid config.json — all fields correct
TEST(ConfigTest, TC_CFG_01_ValidConfig) {
    std::string path = writeTempJson(R"({
        "uid": "agent-001",
        "descr": "web-agent",
        "server_url": "https://xdev.arkcom.ru:9999",
        "poll_interval_sec": 10,
        "retry_count": 3,
        "retry_delay_sec": 5,
        "max_parallel_tasks": 4,
        "task_directory": "./tasks",
        "result_directory": "./results",
        "log_file": "./agent.log",
        "log_level": "info"
    })");

    wa::Config cfg;
    ASSERT_NO_THROW(cfg = wa::Config::load(path));
    EXPECT_EQ(cfg.uid, "agent-001");
    EXPECT_EQ(cfg.server_url, "https://xdev.arkcom.ru:9999");
    EXPECT_EQ(cfg.poll_interval_sec, 10);
    EXPECT_EQ(cfg.log_level, "info");
}

// TC-CFG-02: Missing uid → std::runtime_error
TEST(ConfigTest, TC_CFG_02_MissingUid) {
    std::string path = writeTempJson(R"({
        "server_url": "https://xdev.arkcom.ru:9999"
    })");
    EXPECT_THROW(wa::Config::load(path), std::runtime_error);
}

// TC-CFG-03: Missing server_url → std::runtime_error
TEST(ConfigTest, TC_CFG_03_MissingServerUrl) {
    std::string path = writeTempJson(R"({
        "uid": "agent-001"
    })");
    EXPECT_THROW(wa::Config::load(path), std::runtime_error);
}

// TC-CFG-04: File not found → std::runtime_error
TEST(ConfigTest, TC_CFG_04_FileNotFound) {
    EXPECT_THROW(wa::Config::load("/nonexistent/path/config.json"), std::runtime_error);
}

// TC-CFG-05: Invalid JSON → exception
TEST(ConfigTest, TC_CFG_05_InvalidJson) {
    std::string path = writeTempJson("{ this is not json }");
    EXPECT_THROW(wa::Config::load(path), std::exception);
}

// TC-CFG-06: Default values for optional fields
TEST(ConfigTest, TC_CFG_06_DefaultValues) {
    std::string path = writeTempJson(R"({
        "uid": "agent-001",
        "server_url": "https://xdev.arkcom.ru:9999"
    })");

    wa::Config cfg;
    ASSERT_NO_THROW(cfg = wa::Config::load(path));
    EXPECT_EQ(cfg.poll_interval_sec, 10);
    EXPECT_EQ(cfg.retry_count, 3);
    EXPECT_EQ(cfg.retry_delay_sec, 5);
    EXPECT_EQ(cfg.max_parallel_tasks, 4);
    EXPECT_EQ(cfg.log_level, "info");
    EXPECT_EQ(cfg.task_directory, "./tasks");
    EXPECT_EQ(cfg.result_directory, "./results");
}
