#include "agent.h"
#include "logger.h"
#include "task_handler.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <exception>
#include <ctime>         

namespace wa {

Agent::Agent(Config cfg)
    : cfg_(std::move(cfg))
    , client_(cfg_)
    , task_handler_(cfg_, client_)
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
    WA_LOG_INFO("Agent::run() started, launching poll loop in background");

    running_ = true;
    poll_thread_ = std::thread(&Agent::pollLoop, this);
}

void Agent::stop() noexcept {
    WA_LOG_INFO("Agent::stop() called");
    running_ = false;
}

void Agent::pollLoop() {
    WA_LOG_INFO("Agent::pollLoop() started, interval={}s", cfg_.poll_interval_sec);

    int counter = 0;

    while (running_) {
        try {
            auto task = client_.requestTask();

            if (task.code == 1) {
                WA_LOG_INFO("Получено задание от сервера: {}", task.task_code);
                handleTask(task);
            }
            else {
                if (++counter % 3 == 0) {
                    WA_LOG_INFO("Waiting for tasks...");
                }
            }
        }
        catch (const std::exception& e) {
            WA_LOG_WARN("Не удалось получить задание: {}", e.what());
        }

        for (int i = 0; running_ && i < cfg_.poll_interval_sec; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    WA_LOG_INFO("Agent::pollLoop() stopped");
}

void Agent::handleTask(const TaskInfo& task) {
    task_handler_.process(task);
}

} // namespace wa
