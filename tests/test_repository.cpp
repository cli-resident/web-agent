#include <gtest/gtest.h>
#include <filesystem>

namespace fs = std::filesystem;

// PROJECT_SOURCE_DIR is injected via target_compile_definitions in tests/CMakeLists.txt
#ifndef PROJECT_SOURCE_DIR
#  define PROJECT_SOURCE_DIR "."
#endif

static fs::path root(PROJECT_SOURCE_DIR);

// TC-REP-01: config.json exists in project root
TEST(RepositoryTest, TC_REP_01_ConfigJsonExists) {
    EXPECT_TRUE(fs::exists(root / "config.json"))
        << "Missing: " << (root / "config.json").string();
}

// TC-REP-02: README.md exists
TEST(RepositoryTest, TC_REP_02_ReadmeExists) {
    EXPECT_TRUE(fs::exists(root / "README.md"))
        << "Missing: " << (root / "README.md").string();
}

// TC-REP-03: docs/TZ.md exists
TEST(RepositoryTest, TC_REP_03_TzMdExists) {
    EXPECT_TRUE(fs::exists(root / "docs" / "TZ.md"))
        << "Missing: " << (root / "docs" / "TZ.md").string();
}

// TC-REP-04: All header files exist
TEST(RepositoryTest, TC_REP_04_HeadersExist) {
    const std::vector<std::string> headers = {
        "include/config.h",
        "include/logger.h",
        "include/http_client.h",
        "include/agent.h"
    };
    for (const auto& h : headers) {
        EXPECT_TRUE(fs::exists(root / h))
            << "Missing header: " << (root / h).string();
    }
}

// TC-REP-05: All source files exist
TEST(RepositoryTest, TC_REP_05_SourcesExist) {
    const std::vector<std::string> sources = {
        "src/config.cpp",
        "src/logger.cpp",
        "src/http_client.cpp",
        "src/agent.cpp",
        "src/main.cpp"
    };
    for (const auto& s : sources) {
        EXPECT_TRUE(fs::exists(root / s))
            << "Missing source: " << (root / s).string();
    }
}
