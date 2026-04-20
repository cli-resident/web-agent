#include "config.h"
#include "logger.h"
#include "agent.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

static const char* VERSION = "1.0.0";
static std::atomic<bool> g_running{true};

static void signalHandler(int signal) {
    if (signal == SIGINT) {
        WA_LOG_INFO("Received Ctrl+C, shutting down gracefully...");
        g_running = false;
    }
}

static void printHelp(const char* prog) {
    std::cout << "Usage: " << prog << " [OPTIONS]\n"
              << "\nOptions:\n"
              << "  --config <path>   Path to config file (default: config.json)\n"
              << "  --version         Print version and exit\n"
              << "  --help            Print this help and exit\n";
}

int main(int argc, char* argv[]) {
    std::string config_path = "config.json";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printHelp(argv[0]);
            return 0;
        } else if (arg == "--version" || arg == "-v") {
            std::cout << "WEB-AGENT v" << VERSION << "\n";
            return 0;
        } else if ((arg == "--config" || arg == "-c") && i + 1 < argc) {
            config_path = argv[++i];
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printHelp(argv[0]);
            return 1;
        }
    }

    std::signal(SIGINT, signalHandler);

    wa::Config cfg;
    try {
        cfg = wa::Config::load(config_path);
    } catch (const std::exception& e) {
        std::cerr << "Failed to load config: " << e.what() << "\n";
        return 1;
    }

    try {
        wa::Logger::init(cfg.log_file, cfg.log_level);
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize logger: " << e.what() << "\n";
        return 1;
    }

    WA_LOG_INFO("WEB-AGENT v{} starting, UID={}", VERSION, cfg.uid);

    wa::Agent agent(cfg);
    if (!agent.init()) {
        WA_LOG_ERROR("Agent initialization failed, exiting");
        return 1;
    }

    agent.run();

    WA_LOG_INFO("Agent is running in background. Press Ctrl+C to stop.");

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    WA_LOG_INFO("Shutting down agent...");
    agent.stop();

    WA_LOG_INFO("WEB-AGENT stopped.");
    return 0;
}
