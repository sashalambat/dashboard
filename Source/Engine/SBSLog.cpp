#include "Engine/SBSLog.h"

#include <chrono>
#include <ctime>
#include <iostream>
#include <mutex>

namespace sbs {
namespace {
std::mutex gLogMutex;

const char* levelTag(LogLevel level) {
    switch (level) {
        case LogLevel::Warn: return "WARN";
        case LogLevel::Error: return "ERROR";
        default: return "INFO";
    }
}
} // namespace

void logMessage(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(gLogMutex);
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32) && defined(_MSC_VER)
    localtime_s(&tm, &t);
#elif defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char stamp[32];
    std::strftime(stamp, sizeof(stamp), "%H:%M:%S", &tm);
    auto& out = (level == LogLevel::Error) ? std::cerr : std::cout;
    out << "[" << stamp << "] [" << levelTag(level) << "] " << message << std::endl;
}

void logInfo(const std::string& message) { logMessage(LogLevel::Info, message); }
void logWarn(const std::string& message) { logMessage(LogLevel::Warn, message); }
void logError(const std::string& message) { logMessage(LogLevel::Error, message); }

} // namespace sbs
