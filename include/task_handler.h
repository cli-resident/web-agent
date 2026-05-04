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
    void handleNetworkTask(const TaskInfo& task);

    void sendResult(const TaskInfo& task, int code, const std::string& message,
                    const std::vector<std::string>& files = {});

    struct ProbeResult {
        std::string url;
        bool        success{false};
        long long   latency_ms{-1};
        long        status_code{0};
        long long   bytes{0};
        std::string error;
        std::string timestamp;
    };

    ProbeResult probeTarget(const std::string& url, int timeout_sec);
    std::string buildAiSummary(const std::string& report_json);
    std::string buildFallbackSummary(const std::string& report_json, const std::string& reason);
    std::string sanitizeSessionId(const std::string& session_id) const;
    std::string currentTimestamp() const;
    std::string writeJsonReport(const std::string& session_id, const std::string& json_text);
    std::string writeTextReport(const std::string& session_id, const std::string& text);

    Config& cfg_;
    HttpClient& client_;
};

} // namespace wa
