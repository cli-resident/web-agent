#pragma once
#include <string>
#include <memory>

// Forward declaration — не тянем spdlog в каждый заголовок
namespace spdlog { class logger; }

namespace wa {

/// @brief Обёртка над spdlog. Потокобезопасна.
class Logger {
public:
    /// @brief Инициализировать логгер (вызвать один раз при старте)
    /// @param log_file  Путь к файлу лога
    /// @param level     Уровень: "debug" | "info" | "warn" | "error"
    static void init(const std::string& log_file, const std::string& level = "info");

    /// @brief Получить экземпляр логгера
    static std::shared_ptr<spdlog::logger>& get();

private:
    static std::shared_ptr<spdlog::logger> instance_;
};

/// Макросы для удобного логирования с позицией в коде
#define WA_LOG_INFO(...)  wa::Logger::get()->info(__VA_ARGS__)
#define WA_LOG_WARN(...)  wa::Logger::get()->warn(__VA_ARGS__)
#define WA_LOG_ERROR(...) wa::Logger::get()->error(__VA_ARGS__)
#define WA_LOG_DEBUG(...) wa::Logger::get()->debug(__VA_ARGS__)

} // namespace wa
