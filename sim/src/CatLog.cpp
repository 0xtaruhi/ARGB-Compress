#include "CatLog.hpp"

#include <iostream>

using namespace cat;

#ifdef IN_DEVELOP
CatLog::LogLevel CatLog::log_level_ = CatLog::LogLevel::Debug;
#else
CatLog::LogLevel CatLog::log_level_ = CatLog::LogLevel::Info;
#endif

auto CatLog::logWarning(std::string const &message) -> void {
  if (log_level_ > LogLevel::Warning) {
    return;
  }
  logImpl(LogLevel::Warning, message);
}

auto CatLog::logError(std::string const &message) -> void {
  if (log_level_ > LogLevel::Error) {
    return;
  }
  logImpl(LogLevel::Error, message);
}

auto CatLog::logInfo(std::string const &message) -> void {
  if (log_level_ > LogLevel::Info) {
    return;
  }
  logImpl(LogLevel::Info, message);
}

auto CatLog::logDebug(std::string const &message) -> void {
  if (log_level_ > LogLevel::Debug) {
    return;
  }
  logImpl(LogLevel::Debug, message);
}

auto CatLog::setLogLevel(LogLevel level) -> void { log_level_ = level; }

auto CatLog::getLogLevel() -> LogLevel { return log_level_; }

auto CatLog::logImpl(LogLevel level, std::string const &message) -> void {
  switch (level) {
  case LogLevel::Warning:
    std::cerr << "[WARNING] " << message << std::endl;
    break;
  case LogLevel::Error:
    std::cerr << "[ERROR] " << message << std::endl;
    break;
  case LogLevel::Info:
    std::cout << "[INFO] " << message << std::endl;
    break;
  case LogLevel::Debug:
    std::cout << "[DEBUG] " << message << std::endl;
    break;
  }
}
