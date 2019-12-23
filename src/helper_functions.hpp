#pragma once

#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <deque>

#include "llvm/Support/Error.h"

#ifdef _WIN32
SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE),
               ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

namespace mimium {

template <class ElementType>
static std::string join(std::deque<ElementType>& vec, std::string delim) {
  return std::accumulate(
      std::next(vec.begin()), vec.end(),
      vec.begin()->toString(), 
      [&](std::string a, std::shared_ptr<ElementType>& b) { return std::move(a) + delim + b.toString(); });
};


static std::string join(std::deque<std::string>& vec, std::string delim) {
  return std::accumulate(
      std::next(vec.begin()), vec.end(),
      *(vec.begin()), 
      [&](std::string a, std::string b) { return std::move(a) + delim + b; });
};
template <class T>
static std::string join(std::deque<std::shared_ptr<T>>& vec, std::string delim) {
  return std::accumulate(
      std::next(vec.begin()), vec.end(),
      (*(vec.begin()))->toString(), 
      [&](std::string a, std::shared_ptr<T>& b) { return std::move(a) + delim + b->toString(); });
};

// static std::string join(const std::deque<TypedVal>& vec, std::string delim) {
//   std::string s;
//   for (auto& elem : vec) {
//     s += elem.name;
//     auto endstr = vec.back();
//     if (elem.name != endstr.name) s += delim;
//   }
//   return s;
// };

template <typename T, typename... U>
size_t getAddressfromFun(std::function<T(U...)> f) {
  typedef T(fnType)(U...);
  fnType** fnPointer = f.template target<fnType*>();
  return (size_t)*fnPointer;
}

class Logger {
 public:
  Logger();
  explicit Logger(std::ostream& out);
  virtual ~Logger() = default;
  using REPORT_LEVEL = enum { FATAL = 1, ERROR, WARNING, INFO, DEBUG, TRACE };
  /// @callgraph
  static void debug_log(const std::string& str, REPORT_LEVEL report_level);
  static void debug_log(llvm::Error& err, REPORT_LEVEL report_level) {
    if (bool(err) && report_level <= Logger::current_report_level) {
      llvm::errs() << report_str.at(report_level) << ": " << err << norm
                   << "\n";
    }
  }
  template <typename T>
  static void debug_log(llvm::Expected<T>& expected,
                        REPORT_LEVEL report_level) {
    if (auto err = expected.takeError() &&
                   report_level <= Logger::current_report_level) {
      llvm::errs() << report_str.at(report_level) << ": " << err << norm
                   << "\n";
    }
  }

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
      {INFO, green + "Info"},
      {DEBUG, "Debug"},
      {TRACE, "Trace"}};
};

}  // namespace mimium