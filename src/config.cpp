#include "config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace wa {

Config Config::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Config file not found: " + path);
    }

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error(std::string("Invalid JSON in config: ") + e.what());
    }

    Config cfg;
    cfg.uid        = j.value("uid", "");
    cfg.descr      = j.value("descr", "web-agent");
    cfg.server_url = j.value("server_url", "");
    cfg.poll_interval_sec   = j.value("poll_interval_sec", 10);
    cfg.retry_count         = j.value("retry_count", 3);
    cfg.retry_delay_sec     = j.value("retry_delay_sec", 5);
    cfg.max_parallel_tasks  = j.value("max_parallel_tasks", 4);
    cfg.task_directory      = j.value("task_directory", "./tasks");
    cfg.result_directory    = j.value("result_directory", "./results");
    cfg.log_file            = j.value("log_file", "./agent.log");
    cfg.log_level           = j.value("log_level", "info");
    cfg.access_code         = j.value("access_code", "");
    cfg.source_path         = path;

    cfg.validate();
    return cfg;
}

void Config::validate() const {
    if (uid.empty()) {
        throw std::runtime_error("Config validation error: 'uid' is required");
    }
    if (server_url.empty()) {
        throw std::runtime_error("Config validation error: 'server_url' is required");
    }
}

void Config::save() const {
    if (source_path.empty()) {
        throw std::runtime_error("Config save error: source path is empty");
    }

    nlohmann::json j = {
        {"uid", uid},
        {"descr", descr},
        {"server_url", server_url},
        {"poll_interval_sec", poll_interval_sec},
        {"retry_count", retry_count},
        {"retry_delay_sec", retry_delay_sec},
        {"max_parallel_tasks", max_parallel_tasks},
        {"task_directory", task_directory},
        {"result_directory", result_directory},
        {"log_file", log_file},
        {"log_level", log_level},
        {"access_code", access_code}
    };

    std::ofstream out(source_path);
    if (!out.is_open()) {
        throw std::runtime_error("Config save error: cannot open file " + source_path);
    }
    out << std::setw(2) << j << '\n';
}

} // namespace wa
