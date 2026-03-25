#pragma once
#include "http_client.h"
#include "config.h"

namespace wa {

class TaskHandler {
public:
    TaskHandler(Config& cfg, HttpClient& client);

    void process(const TaskInfo& task);

private:
    void handleTimeout(const TaskInfo& task);
    void handleConf(const TaskInfo& task);
    void handleFile(const TaskInfo& task);
    void handleExec(const TaskInfo& task);

    void sendResult(const TaskInfo& task, int code, const std::string& message,
                    const std::vector<std::string>& files = {});

    Config& cfg_;
    HttpClient& client_;
};

} // namespace wa
