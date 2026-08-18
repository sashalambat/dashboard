#pragma once

#include <string>

namespace sbs {

enum class LogLevel { Info, Warn, Error };

void logMessage(LogLevel level, const std::string& message);
void logInfo(const std::string& message);
void logWarn(const std::string& message);
void logError(const std::string& message);

} // namespace sbs
