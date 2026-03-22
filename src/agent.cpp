#include "agent.h"
#include "logger.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <exception>

// TODO: ЛР №3 — реализация цикла опроса агента

namespace wa {

Agent::Agent(Config cfg)
    : cfg_(std::move(cfg))
    , client_(cfg_)
{
    WA_LOG_DEBUG("Agent constructed, UID={}", cfg_.uid);
}

Agent::~Agent() noexcept {
    stop();
    if (poll_thread_.joinable()) {
        poll_thread_.join();
    }
}

bool Agent::init() {
    if (!cfg_.access_code.empty()) {
        WA_LOG_INFO("Agent::init() — using preset access_code for UID={}", cfg_.uid);
        return true;
    }

    WA_LOG_INFO("Agent::init() — registering UID={}", cfg_.uid);
    auto resp = client_.registerAgent();
    if (resp.code == 0) {
        cfg_.access_code = resp.access_code;
        WA_LOG_INFO("Agent registered, access_code={}", cfg_.access_code);
        if (!cfg_.access_code.empty()) {
            try {
                cfg_.save();
                WA_LOG_INFO("Config updated with new access_code");
            } catch (const std::exception& e) {
                WA_LOG_WARN("Failed to persist access_code: {}", e.what());
            }
        }
        return true;
    }

    if (resp.code == -3) {
        WA_LOG_ERROR("Agent already registered but access_code is not provided. "
                     "Set access_code in config.json to reuse the existing token.");
        return false;
    }

    WA_LOG_ERROR("Agent registration failed, code={}, msg={}", resp.code, resp.msg);
    return false;
}

void Agent::run() {
    // TODO: ЛР №3 — полная реализация цикла опроса
    WA_LOG_INFO("Agent::run() — stub, starting poll loop");
    running_ = true;
    poll_thread_ = std::thread(&Agent::pollLoop, this);
    if (poll_thread_.joinable()) {
        poll_thread_.join();
    }
}

void Agent::stop() noexcept {
    WA_LOG_INFO("Agent::stop() called");
    running_ = false;
}

void Agent::pollLoop() {
    WA_LOG_INFO("Agent::pollLoop() started, interval={}s", cfg_.poll_interval_sec);
    while (running_) {
        try {
            auto task = client_.requestTask();
            if (task.code == 1) {
                WA_LOG_INFO("Получено задание: task_code={}, session_id={}, status={}",
                            task.task_code, task.session_id, task.status);
                // Обработка будет добавлена на следующих этапах
            } else if (task.code == 0 || task.status == "WAIT") {
                WA_LOG_DEBUG("Новых задач нет (status=WAIT)");
            } else {
                WA_LOG_WARN("Необычный ответ сервера: code={}, status={}, msg={}",
                            task.code, task.status, task.msg);
            }
        } catch (const std::exception& e) {
            WA_LOG_ERROR("Ошибка при запросе задания: {}", e.what());
        }

        for (int i = 0; running_ && i < cfg_.poll_interval_sec; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    WA_LOG_INFO("Agent::pollLoop() завершён");
}

void Agent::handleTask(const TaskInfo& task) {
    // TODO: ЛР №3/4 — реализация обработки заданий
    WA_LOG_INFO("Agent::handleTask() — stub, task_code={}, session_id={}",
                task.task_code, task.session_id);
}

} 
