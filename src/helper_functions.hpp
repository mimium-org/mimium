#pragma once

#include <iostream>
#include <map>
#include <string>

#ifdef _WIN32
SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE),
               ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

namespace mimium {

template<typename T, typename... U>
size_t getAddressfromFun(std::function<T(U...)> f) {
    typedef T(fnType)(U...);
    fnType ** fnPointer = f.template target<fnType*>();
    return (size_t) *fnPointer;
}


class Logger {
 public:
  Logger();
  explicit Logger(std::ostream& out);
  virtual ~Logger() = default;
  using REPORT_LEVEL = enum { FATAL = 1, ERROR, WARNING, INFO, DEBUG, TRACE };
  /// @callgraph
  static void debug_log(const std::string& str, REPORT_LEVEL report_level);
  inline void setoutput(std::ostream& out) { Logger::output = &out; }
  static REPORT_LEVEL current_report_level;

 private:
  static std::ostream* output;

  static inline const std::string red = "\033[1;31m";
  static inline const std::string green = "\033[1;32m";

  static inline const std::string yellow = "\033[1;33m";
  static inline const std::string norm = "\033[0m";
  static inline const std::map<Logger::REPORT_LEVEL, std::string> report_str = {
      {FATAL, red + "Fatal"},
      {ERROR, red + "Error"},
      {WARNING, yellow + "Warning"},
      {INFO, green+ "Info"},
      {DEBUG, "Debug"},
      {TRACE, "Trace"}};
};


}  // namespace mimium