#include "logger.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <vector>
#include <memory>
#include <stdexcept>

namespace wa {

std::shared_ptr<spdlog::logger> Logger::instance_;

void Logger::init(const std::string& log_file, const std::string& level) {
    std::vector<spdlog::sink_ptr> sinks;

    // Stdout sink (coloured)
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sinks.push_back(stdout_sink);

    // Rotating file sink: 5 MB, 3 rotations
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        log_file, 5 * 1024 * 1024, 3);
    sinks.push_back(file_sink);

    instance_ = std::make_shared<spdlog::logger>("wa", sinks.begin(), sinks.end());
    instance_->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");

    // Set log level
    if (level == "debug") {
        instance_->set_level(spdlog::level::debug);
    } else if (level == "warn" || level == "warning") {
        instance_->set_level(spdlog::level::warn);
    } else if (level == "error") {
        instance_->set_level(spdlog::level::err);
    } else {
        instance_->set_level(spdlog::level::info);
    }

    spdlog::register_logger(instance_);
}

std::shared_ptr<spdlog::logger>& Logger::get() {
    if (!instance_) {
        throw std::runtime_error("Logger not initialized. Call Logger::init() first.");
    }
    return instance_;
}

} // namespace wa
