#pragma once
#include <string>

namespace wa {

/// @brief Конфигурация агента, загружается из JSON-файла
struct Config {
    std::string uid;
    std::string descr              = "web-agent";
    std::string server_url;
    int    poll_interval_sec       = 10;
    int    retry_count             = 3;
    int    retry_delay_sec         = 5;
    int    max_parallel_tasks      = 4;
    std::string task_directory     = "./tasks";
    std::string result_directory   = "./results";
    std::string log_file           = "./agent.log";
    std::string log_level          = "info";
    std::string access_code;  ///< Заполняется после регистрации или читается из config.json
    std::string source_path;   ///< Путь к конфигурационному файлу (для сохранения)

    /// @brief Загрузить конфигурацию из JSON-файла
    /// @throws std::runtime_error если файл не найден или поля uid/server_url пустые
    static Config load(const std::string& path);

    /// @brief Проверить корректность конфига
    /// @throws std::runtime_error при ошибке валидации
    void validate() const;

    /// @brief Сохранить текущие значения конфигурации в исходный файл
    /// @throws std::runtime_error если путь неизвестен или запись не удалась
    void save() const;
};

} // namespace wa
