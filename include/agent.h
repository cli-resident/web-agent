#pragma once
#include "config.h"
#include "http_client.h"
#include <atomic>
#include <thread>

namespace wa {

/// @brief Основной класс агента. Управляет жизненным циклом и циклом опроса.
class Agent {
public:
    explicit Agent(Config cfg);
    ~Agent() noexcept;

    /// @brief Инициализация: проверка сервера, регистрация
    /// @return true при успехе или code==-3 (уже зарегистрирован)
    bool init();

    /// @brief Запуск цикла опроса (блокирующий вызов)
    void run();

    /// @brief Потокобезопасная остановка агента
    void stop() noexcept;

private:
    void pollLoop();
    void handleTask(const TaskInfo& task);

    Config             cfg_;
    HttpClient         client_;
    std::atomic<bool>  running_{false};
    std::thread        poll_thread_;
};

} // namespace wa
