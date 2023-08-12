#ifndef CAT_LOG_HPP
#define CAT_LOG_HPP

#include <string>

namespace cat {

class CatLog {
public:
  enum class LogLevel { Debug, Info, Warning, Error };

  static auto logError(std::string const &message) -> void;
  static auto logWarning(std::string const &message) -> void;
  static auto logInfo(std::string const &message) -> void;
  static auto logDebug(std::string const &message) -> void;

  static auto setLogLevel(LogLevel level) -> void;
  static auto getLogLevel() -> LogLevel;

private:
  static auto logImpl(LogLevel level, std::string const &message) -> void;

  static LogLevel log_level_;
};

} // namespace cat

#endif
