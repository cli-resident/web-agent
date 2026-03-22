#pragma once
#include "config.h"
#include <string>
#include <vector>

namespace wa {

/// @brief Ответ на регистрацию агента
struct RegResponse {
    int         code;         ///< 0=успех, -3=уже зарегистрирован, <0=ошибка
    std::string msg;
    std::string access_code;  ///< Заполняется при code==0
};

/// @brief Задание от сервера
struct TaskInfo {
    int         code;         ///< 1=задание, 0=ожидание, <0=ошибка
    std::string task_code;    ///< Тип задания: CONF, CMD, EXEC, FILE
    std::string options;      ///< Параметры задания
    std::string session_id;   ///< UUID сессии для отправки результата
    std::string status;       ///< RUN | WAIT
    std::string msg;
};

/// @brief Ответ на отправку результата
struct ResultResponse {
    int         code;
    std::string msg;
};

/// @brief HTTP-клиент для взаимодействия с сервером WEB-AGENT API
class HttpClient {
public:
    explicit HttpClient(Config& cfg);

    /// @brief POST /api/wa_reg/ — регистрация агента
    RegResponse registerAgent();

    /// @brief POST /api/wa_task/ — запрос задания
    TaskInfo requestTask();

    /// @brief POST /api/wa_result/ — отправка результата (multipart/form-data)
    ResultResponse sendResult(
        const std::string&              session_id,
        int                             result_code,
        const std::string&              message,
        const std::vector<std::string>& file_paths = {}
    );

private:
    Config& cfg_;
    std::string buildUrl(const std::string& endpoint) const;
};

} // namespace wa
