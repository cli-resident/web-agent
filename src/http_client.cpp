#include "http_client.h"
#include "logger.h"
#include <spdlog/spdlog.h>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <thread>
#include <stdexcept>
#include <functional>

namespace wa {

namespace {

constexpr auto kRegEndpoint    = "/app/webagent1/api/wa_reg/";
constexpr auto kTaskEndpoint   = "/app/webagent1/api/wa_task/";
constexpr auto kResultEndpoint = "/app/webagent1/api/wa_result/";

int parseResponseCode(const nlohmann::json& body) {
    if (auto it = body.find("code_responce"); it != body.end()) {
        if (it->is_string()) return std::stoi(it->get<std::string>());
        if (it->is_number_integer()) return it->get<int>();
    }
    if (auto it = body.find("code"); it != body.end() && it->is_number_integer()) {
        return it->get<int>();
    }
    return -999;
}

nlohmann::json toJson(const std::string& text) {
    try {
        if (text.empty()) return {};
        return nlohmann::json::parse(text);
    } catch (...) {
        return {};
    }
}

void ensureOk(const cpr::Response& resp, bool treat_empty_ok = false) {
    if (resp.error.code != cpr::ErrorCode::OK) {
        throw std::runtime_error("Curl error: " + resp.error.message);
    }
    if (resp.status_code < 200 || resp.status_code >= 300) {
        throw std::runtime_error("HTTP " + std::to_string(resp.status_code));
    }
    if (!treat_empty_ok && resp.text.empty()) {
        throw std::runtime_error("Empty response body");
    }
}

}

HttpClient::HttpClient(Config& cfg) : cfg_(cfg) {
    WA_LOG_DEBUG("HttpClient constructed for server: {}", cfg_.server_url);
}

std::string HttpClient::buildUrl(const std::string& endpoint) const {
    return cfg_.server_url + endpoint;
}

cpr::Response HttpClient::performWithRetry(
    std::function<cpr::Response()> makeRequest,
    bool treat_empty_ok
) const {
    int attempts = 0;
    const int max_attempts = cfg_.retry_count + 1;
    auto delay = std::chrono::seconds(cfg_.retry_delay_sec);

    while (attempts < max_attempts) {
        try {
            auto resp = makeRequest();
            ensureOk(resp, treat_empty_ok);
            return resp;
        }
        catch (const std::exception& e) {
            attempts++;

            WA_LOG_WARN("Request failed (attempt {}/{}): {}", attempts, max_attempts, e.what());

            if (attempts >= max_attempts) {
                WA_LOG_ERROR("All retry attempts failed after {} tries. Last error: {}",
                             max_attempts, e.what());
                throw std::runtime_error("Request failed after all retries");
            }

            WA_LOG_INFO("Backoff: waiting {}s before retry #{}", delay.count(), attempts + 1);

            std::this_thread::sleep_for(delay);
            delay = std::min(delay * 2, std::chrono::seconds(60));
        }
    }

    throw std::runtime_error("Unexpected exit from retry loop");
}

RegResponse HttpClient::registerAgent() {
    nlohmann::json payload = {{"UID", cfg_.uid}, {"descr", cfg_.descr}};

    auto makeReq = [this, &payload]() {
        return cpr::Post(
            cpr::Url{buildUrl(kRegEndpoint)},
            cpr::Header{{"Content-Type", "application/json"}},
            cpr::Body{payload.dump()},
            cpr::ConnectTimeout{std::chrono::seconds(cfg_.connect_timeout_sec)},
            cpr::Timeout{std::chrono::seconds(cfg_.request_timeout_sec)}
        );
    };

    auto resp = performWithRetry(makeReq, false);
    auto body = toJson(resp.text);

    RegResponse r{};
    r.code = parseResponseCode(body);
    r.msg = body.value("msg", "");
    r.access_code = body.value("access_code", "");
    return r;
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

    auto makeReq = [this, &payload]() {
        return cpr::Post(
            cpr::Url{buildUrl(kTaskEndpoint)},
            cpr::Header{{"Content-Type", "application/json"}},
            cpr::Body{payload.dump()},
            cpr::ConnectTimeout{std::chrono::seconds(cfg_.connect_timeout_sec)},
            cpr::Timeout{std::chrono::seconds(cfg_.request_timeout_sec)}
        );
    };

    auto resp = performWithRetry(makeReq, false);
    auto body = toJson(resp.text);

    TaskInfo info{};
    info.code       = parseResponseCode(body);
    info.task_code  = body.value("task_code", "");
    info.options    = body.value("options", "");
    info.session_id = body.value("session_id", "");
    info.status     = body.value("status", "");
    info.msg        = body.value("msg", "");

//    WA_LOG_DEBUG("HttpClient::requestTask() -> code={}, status={}, task_code={}",
//                 info.code, info.status, info.task_code);
    return info;
}

ResultResponse HttpClient::sendResult(const std::string& /*session_id*/,
                                      int /*result_code*/,
                                      const std::string& /*message*/,
                                      const std::vector<std::string>& /*file_paths*/) {
    ResultResponse r{};
    r.code = 0;
    r.msg = "sendResult not implemented yet";
    return r;
}

} // namespace wa
