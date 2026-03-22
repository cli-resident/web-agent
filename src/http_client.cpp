#include "http_client.h"
#include "logger.h"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <sstream>
#include <stdexcept>

namespace {
constexpr auto kRegEndpoint    = "/app/webagent1/api/wa_reg/";
constexpr auto kTaskEndpoint   = "/app/webagent1/api/wa_task/";
constexpr auto kResultEndpoint = "/app/webagent1/api/wa_result/";

constexpr auto kConnectTimeoutMs = 5000;
constexpr auto kRequestTimeoutMs = 10000;
constexpr auto kUploadTimeoutMs  = 30000;

int parseResponseCode(const nlohmann::json& body) {
    if (auto it = body.find("code_responce"); it != body.end()) {
        if (it->is_string()) {
            return std::stoi(it->get<std::string>());
        }
        if (it->is_number_integer()) {
            return it->get<int>();
        }
    }
    if (auto it = body.find("code"); it != body.end() && it->is_number_integer()) {
        return it->get<int>();
    }
    throw std::runtime_error("HttpClient: response does not contain code_responce field");
}

nlohmann::json toJson(const std::string& text) {
    try {
        if (text.empty()) {
            throw std::runtime_error("empty body");
        }
        return nlohmann::json::parse(text);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("HttpClient: failed to parse JSON response: ") + e.what());
    }
}

void ensureOk(const cpr::Response& resp, bool treat_empty_ok = false) {
    if (resp.error.code != cpr::ErrorCode::OK) {
        throw std::runtime_error(
            std::string("HTTP error: ") + resp.error.message);
    }
    if (resp.status_code < 200 || resp.status_code >= 300) {
        std::ostringstream oss;
        oss << "HTTP status " << resp.status_code;
        throw std::runtime_error(oss.str());
    }
    if (!treat_empty_ok && resp.text.empty()) {
        throw std::runtime_error("HTTP response body is empty");
    }
}
} // namespace

namespace wa {

HttpClient::HttpClient(Config& cfg) : cfg_(cfg) {
    WA_LOG_DEBUG("HttpClient constructed for server: {}", cfg_.server_url);
}

std::string HttpClient::buildUrl(const std::string& endpoint) const {
    return cfg_.server_url + endpoint;
}

RegResponse HttpClient::registerAgent() {
    WA_LOG_DEBUG("HttpClient::registerAgent() -- sending request");
    nlohmann::json payload = {
        {"UID", cfg_.uid},
        {"descr", cfg_.descr}
    };

    auto response = cpr::Post(
        cpr::Url{buildUrl(kRegEndpoint)},
        cpr::Header{{"Content-Type", "application/json"}},
        cpr::Body{payload.dump()},
        cpr::ConnectTimeout{kConnectTimeoutMs},
        cpr::Timeout{kRequestTimeoutMs}
    );

    ensureOk(response);
    auto body = toJson(response.text);

    RegResponse result{};
    result.code = parseResponseCode(body);
    result.msg  = body.value("msg", std::string{});
    if (body.contains("access_code")) {
        result.access_code = body.value("access_code", std::string{});
    }

    WA_LOG_INFO("HttpClient::registerAgent() finished with code={}, msg={}", result.code, result.msg);
    return result;
}

TaskInfo HttpClient::requestTask() {
    if (cfg_.access_code.empty()) {
        throw std::runtime_error("HttpClient::requestTask() called without access_code");
    }

    nlohmann::json payload = {
        {"UID", cfg_.uid},
        {"descr", cfg_.descr},
        {"access_code", cfg_.access_code}
    };

    auto response = cpr::Post(
        cpr::Url{buildUrl(kTaskEndpoint)},
        cpr::Header{{"Content-Type", "application/json"}},
        cpr::Body{payload.dump()},
        cpr::ConnectTimeout{kConnectTimeoutMs},
        cpr::Timeout{kRequestTimeoutMs}
    );

    ensureOk(response);
    auto body = toJson(response.text);

    TaskInfo info{};
    info.code       = parseResponseCode(body);
    info.task_code  = body.value("task_code", std::string{});
    info.options    = body.value("options", std::string{});
    info.session_id = body.value("session_id", std::string{});
    info.status     = body.value("status", std::string{});
    info.msg        = body.value("msg", std::string{});

    WA_LOG_DEBUG("HttpClient::requestTask() -> code={}, status={}, task_code={}",
                 info.code, info.status, info.task_code);
    return info;
}

ResultResponse HttpClient::sendResult(
    const std::string& session_id,
    int result_code,
    const std::string& message,
    const std::vector<std::string>& file_paths)
{
    if (cfg_.access_code.empty()) {
        throw std::runtime_error("HttpClient::sendResult() called without access_code");
    }

    nlohmann::json meta = {
        {"UID", cfg_.uid},
        {"access_code", cfg_.access_code},
        {"message", message},
        {"files", static_cast<int>(file_paths.size())},
        {"session_id", session_id}
    };

    cpr::Multipart multipart({});
    multipart.parts.emplace_back(cpr::Part{"result_code", std::to_string(result_code)});
    multipart.parts.emplace_back(cpr::Part{"result", meta.dump()});

    namespace fs = std::filesystem;
    for (size_t i = 0; i < file_paths.size(); ++i) {
        const auto& file = file_paths[i];
        if (!fs::exists(file)) {
            WA_LOG_WARN("Result file not found: {}", file);
            continue;
        }
        std::string field = "file" + std::to_string(i + 1);
        multipart.parts.emplace_back(cpr::Part{field, cpr::File{file}});
    }

    auto response = cpr::Post(
        cpr::Url{buildUrl(kResultEndpoint)},
        multipart,
        cpr::ConnectTimeout{kConnectTimeoutMs},
        cpr::Timeout{kUploadTimeoutMs}
    );

    ensureOk(response, true);
    auto body = response.text.empty() ? nlohmann::json{} : toJson(response.text);

    ResultResponse result{};
    if (!body.empty()) {
        result.code = parseResponseCode(body);
        result.msg  = body.value("msg", std::string{});
    } else {
        result.code = 0;
        result.msg  = "";
    }

    WA_LOG_INFO("HttpClient::sendResult() finished with code={} msg={}", result.code, result.msg);
    return result;
}

} // namespace wa
