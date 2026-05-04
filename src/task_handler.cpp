#include "task_handler.h"
#include "logger.h"

#include <cctype>
#include <chrono>
#include <cpr/cpr.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <sstream>
#include <ctime>

namespace {
constexpr auto kDefaultTimeoutSec = 8;

// Настройки интеграции с внешним AI (Mistral Chat API).
constexpr auto kAiEndpoint = "https://api.mistral.ai/v1/chat/completions";
constexpr auto kAiApiKey   = "5kk6IDD9mNINAos8ceJaHLAIoQXic4O1";
constexpr auto kAiModel    = "mistral-small-latest";
constexpr auto kAiSystemPrompt =
    "Ты — ассистент сетевой диагностики. "
    "Тебе дают JSON-отчет по endpoint'ам. Отвечай ТОЛЬКО на русском и СТРОГО по шаблону ниже. "
    "Никакого markdown, никаких таблиц, никаких списков с разной структурой, никаких лишних разделов. "
    "Шаблон ответа (строго сохранить заголовки и порядок): "
    "ИТОГ: <OK|WARN|CRIT> "
    "КРАТКО: <1 короткое предложение до 140 символов> "
    "ОБЪЕКТЫ: "
    "- <url_1> | <OK|WARN|CRIT> | code=<int> | latency=<int>ms | причина=<кратко> "
    "- <url_2> | <OK|WARN|CRIT> | code=<int> | latency=<int>ms | причина=<кратко> "
    "ПРОБЛЕМЫ: "
    "1) <если есть проблема: кратко и конкретно; если нет, напиши 'нет критичных проблем'> "
    "2) <опционально, если есть вторая> "
    "3) <опционально, если есть третья> "
    "РЕКОМЕНДАЦИИ: "
    "1) <короткое действие в повелительной форме> "
    "2) <короткое действие> "
    "3) <короткое действие или 'доп. действий не требуется'> "
    "Правила оценки: "
    "OK: success=true, code 200..399, latency<=1000. "
    "WARN: success=true и (latency>1000 или code 400..499). "
    "CRIT: success=false или code>=500 или error непустой. "
    "Общий ИТОГ: CRIT если есть хотя бы один CRIT, иначе WARN если есть WARN, иначе OK. "
    "Требования к стилю: пиши коротко, технически, без воды; используй только факты из входного JSON; "
    "не придумывай метрики; для каждого объекта обязательна строка в разделе ОБЪЕКТЫ; "
    "если поле отсутствует, подставь code=0, latency=-1, причина='нет данных'. "
    "Если вход невалидный или пустой, верни строго: "
    "ИТОГ: CRIT "
    "КРАТКО: входной отчет некорректен "
    "ОБЪЕКТЫ: "
    "- нет данных | CRIT | code=0 | latency=-1ms | причина=ошибка парсинга "
    "ПРОБЛЕМЫ: "
    "1) входной JSON не распознан "
    "РЕКОМЕНДАЦИИ: "
    "1) проверь формат отчета и повтори запрос.";

std::string isoTimestamp() {
    using namespace std::chrono;
    auto now   = system_clock::now();
    auto tt    = system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &tm);
    return std::string(buffer);
}
} // namespace

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
        handleNetworkTask(task);
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
    try {
        namespace fs = std::filesystem;
        fs::path out_path = "test_payload.json";
        std::ofstream out(out_path, std::ios::trunc);
        if (!out.is_open()) {
            sendResult(task, -3, "FILE task failed: cannot open test_payload.json");
            return;
        }

        auto parsed = nlohmann::json::parse(task.options, nullptr, false);
        if (!parsed.is_discarded()) {
            out << parsed.dump(2) << '\n';
        } else {
            out << task.options;
            if (task.options.empty() || task.options.back() != '\n') {
                out << '\n';
            }
        }

        sendResult(task, 0, "FILE task saved payload to test_payload.json");
    } catch (const std::exception& e) {
        sendResult(task, -3, std::string("FILE task failed: ") + e.what());
    }
}

void TaskHandler::handleNetworkTask(const TaskInfo& task) {
    if (task.options.empty()) {
        sendResult(task, -2, "TASK options are empty");
        return;
    }

    nlohmann::json options;
    try {
        options = nlohmann::json::parse(task.options);
    } catch (const std::exception& e) {
        sendResult(task, -2, std::string("Invalid TASK options JSON: ") + e.what());
        return;
    }

    const auto& targets_json = options.value("targets", nlohmann::json::array());
    if (!targets_json.is_array() || targets_json.empty()) {
        sendResult(task, -2, "No targets provided for TASK");
        return;
    }

    int default_timeout = options.value("timeout_sec", kDefaultTimeoutSec);
    nlohmann::json report;
    report["session_id"]  = task.session_id;
    report["generated_at"] = currentTimestamp();
    report["targets"]     = nlohmann::json::array();

    for (const auto& target : targets_json) {
        if (!target.is_object()) {
            continue;
        }
        std::string url;
        if (target.contains("url") && target["url"].is_string()) {
            url = target["url"].get<std::string>();
        } else if (target.contains("host") && target["host"].is_string()) {
            url = target["host"].get<std::string>();
        }
        if (url.empty()) {
            continue;
        }
        int timeout_sec = default_timeout;
        if (target.contains("timeout_sec") && target["timeout_sec"].is_number_integer()) {
            timeout_sec = target["timeout_sec"].get<int>();
        }
        auto probe = probeTarget(url, timeout_sec);
        nlohmann::json entry = {
            {"url", probe.url},
            {"success", probe.success},
            {"latency_ms", probe.latency_ms},
            {"status_code", probe.status_code},
            {"bytes", probe.bytes},
            {"error", probe.error},
            {"timestamp", probe.timestamp}
        };
        report["targets"].push_back(entry);
    }

    if (report["targets"].empty()) {
        sendResult(task, -2, "Targets parsed but none were valid");
        return;
    }

    const auto report_text = report.dump(2);
    auto report_path       = writeJsonReport(task.session_id, report_text);
    const auto summary     = buildAiSummary(report.dump());
    auto summary_path      = writeTextReport(task.session_id, summary);

    std::vector<std::string> files;
    if (!report_path.empty()) {
        files.push_back(report_path);
    }
    if (!summary_path.empty()) {
        files.push_back(summary_path);
    }

    sendResult(task, 0, summary, files);
}

TaskHandler::ProbeResult TaskHandler::probeTarget(const std::string& url, int timeout_sec) {
    ProbeResult result;
    result.url       = url;
    result.timestamp = currentTimestamp();
    const int timeout_ms = timeout_sec > 0 ? timeout_sec * 1000 : kDefaultTimeoutSec * 1000;

    try {
        auto start = std::chrono::steady_clock::now();
        auto resp  = cpr::Get(
            cpr::Url{url},
            cpr::Timeout{timeout_ms},
            cpr::VerifySsl{true}
        );
        auto end              = std::chrono::steady_clock::now();
        result.latency_ms     = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        result.status_code    = resp.status_code;
        result.bytes          = static_cast<long long>(resp.text.size());

        if (resp.error.code != cpr::ErrorCode::OK) {
            result.success = false;
            result.error   = resp.error.message.empty() ? "HTTP error" : resp.error.message;
            return result;
        }

        if (resp.status_code >= 200 && resp.status_code < 400) {
            result.success = true;
        } else {
            result.success = false;
            std::ostringstream oss;
            oss << "HTTP " << resp.status_code;
            result.error = oss.str();
        }
    } catch (const std::exception& e) {
        result.success = false;
        result.error   = e.what();
    }
    return result;
}

std::string TaskHandler::buildAiSummary(const std::string& report_json) {
    nlohmann::json payload = {
        {"model", kAiModel},
        {"temperature", 0.2},
        {"messages", nlohmann::json::array({
            {{"role", "system"}, {"content", kAiSystemPrompt}},
            {{"role", "user"}, {"content", report_json}}
        })}
    };

    try {
        auto resp = cpr::Post(
            cpr::Url{kAiEndpoint},
            cpr::Header{
                {"Content-Type", "application/json"},
                {"Authorization", std::string("Bearer ") + kAiApiKey}
            },
            cpr::Body{payload.dump()},
            cpr::Timeout{10000}
        );

        if (resp.error.code != cpr::ErrorCode::OK) {
            throw std::runtime_error(resp.error.message);
        }
        if (resp.status_code < 200 || resp.status_code >= 300) {
            std::ostringstream oss;
            oss << "AI HTTP status " << resp.status_code;
            throw std::runtime_error(oss.str());
        }

        auto body = nlohmann::json::parse(resp.text);
        if (body.contains("choices") && body["choices"].is_array() && !body["choices"].empty()) {
            const auto& choice = body["choices"].front();
            if (choice.contains("message") && choice["message"].contains("content")) {
                return choice["message"]["content"].get<std::string>();
            }
        }
        return buildFallbackSummary(report_json, "AI response format unexpected");
    } catch (const std::exception& e) {
        WA_LOG_WARN("AI summary request failed: {}", e.what());
        return buildFallbackSummary(report_json, e.what());
    }
}

std::string TaskHandler::buildFallbackSummary(const std::string& report_json, const std::string& reason) {
    nlohmann::json report = nlohmann::json::parse(report_json, nullptr, false);
    if (report.is_discarded()) {
        return "AI недоступен (" + reason + "). См. полный отчёт.";
    }

    std::ostringstream oss;
    oss << "AI недоступен (" << reason << "). Сводка по целям:\n";
    if (report.contains("targets") && report["targets"].is_array()) {
        for (const auto& target : report["targets"]) {
            const std::string url = target.value("url", "");
            const bool success    = target.value("success", false);
            const auto latency    = target.value("latency_ms", -1);
            const auto status     = target.value("status_code", 0);
            const auto error      = target.value("error", "");
            oss << " - " << url << ": "
                << (success ? "OK" : "FAIL")
                << ", latency=" << latency << "ms"
                << ", status=" << status;
            if (!error.empty()) {
                oss << ", error=" << error;
            }
            oss << "\n";
        }
    }
    return oss.str();
}

std::string TaskHandler::sanitizeSessionId(const std::string& session_id) const {
    std::string safe = session_id;
    for (char& ch : safe) {
        if (!std::isalnum(static_cast<unsigned char>(ch))) {
            ch = '_';
        }
    }
    if (safe.empty()) {
        safe = "session";
    }
    return safe;
}

std::string TaskHandler::currentTimestamp() const {
    return isoTimestamp();
}

std::string TaskHandler::writeJsonReport(const std::string& session_id, const std::string& json_text) {
    namespace fs = std::filesystem;
    try {
        fs::create_directories(cfg_.result_directory);
        auto filename = "netdiag_" + sanitizeSessionId(session_id) + ".json";
        auto path     = fs::path(cfg_.result_directory) / filename;
        std::ofstream ofs(path);
        ofs << json_text;
        return path.string();
    } catch (const std::exception& e) {
        WA_LOG_WARN("Failed to write JSON report: {}", e.what());
        return {};
    }
}

std::string TaskHandler::writeTextReport(const std::string& session_id, const std::string& text) {
    namespace fs = std::filesystem;
    try {
        fs::create_directories(cfg_.result_directory);
        auto filename = "netdiag_" + sanitizeSessionId(session_id) + "_summary.txt";
        auto path     = fs::path(cfg_.result_directory) / filename;
        std::ofstream ofs(path);
        ofs << text;
        return path.string();
    } catch (const std::exception& e) {
        WA_LOG_WARN("Failed to write summary report: {}", e.what());
        return {};
    }
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
