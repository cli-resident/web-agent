#include "task_handler.h"
#include "logger.h"
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace wa {

TaskHandler::TaskHandler(Config& cfg, HttpClient& client)
    : cfg_(cfg), client_(client) {}

void TaskHandler::process(const TaskInfo& task) {
    if (task.task_code == "TIMEOUT") {
        handleTimeout(task);
    } else if (task.task_code == "CONF") {
        handleConf(task);
    } else if (task.task_code == "FILE") {
        handleFile(task);
    } else if (task.task_code == "TASK" || task.task_code == "EXEC" || task.task_code == "CMD") {
        handleExec(task);
    } else {
        sendResult(task, -1, "Unknown task_code: " + task.task_code);
    }
}

void TaskHandler::handleTimeout(const TaskInfo& task) {
    try {
        int new_interval = std::stoi(task.options);
        if (new_interval <= 0) {
            sendResult(task, -2, "Invalid timeout value: " + task.options);
            return;
        }
        cfg_.poll_interval_sec = new_interval;
        cfg_.save();
        sendResult(task, 0, "poll_interval_sec set to " + std::to_string(new_interval));
    } catch (const std::exception& e) {
        sendResult(task, -2, std::string("Failed to parse timeout: ") + e.what());
    }
}

void TaskHandler::handleConf(const TaskInfo& task) {
    sendResult(task, 0, "CONF task not implemented yet");
}

void TaskHandler::handleFile(const TaskInfo& task) {
    sendResult(task, 0, "FILE task not implemented yet");
}

void TaskHandler::handleExec(const TaskInfo& task) {
    sendResult(task, 0, "TASK/EXEC/CMD task not implemented yet");
}

void TaskHandler::sendResult(const TaskInfo& task, int code,
                             const std::string& message,
                             const std::vector<std::string>& files) {
    try {
        auto resp = client_.sendResult(task.session_id, code, message, files);
        if (resp.code == 0) {
            WA_LOG_INFO("Результат задания {} отправлен (code={}, msg={})",
                        task.session_id, code, message);
        } else {
            WA_LOG_WARN("Отправка результата задания {} вернула code_responce={}, msg={}",
                        task.session_id, resp.code, resp.msg);
        }
    } catch (const std::exception& e) {
        WA_LOG_ERROR("Не удалось отправить результат задания {}: {}", task.session_id, e.what());
    }
}

} // namespace wa
