/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <condition_variable>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <stack>
#include <utility>  //pair
#include <vector>
#include "abstractions.hpp"
#include "export.hpp"

#ifdef _WIN32
// SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
#if defined(__clang__)
#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
// code that builds only under AddressSanitizer
#define NO_SANITIZE __attribute__((no_sanitize("address", "undefined")))
#endif
#endif
#ifndef NO_SANITIZE
#define NO_SANITIZE
#endif

namespace mimium {

#ifdef MIMIUM_DEBUG_BUILD
#define MMMASSERT(cond, message) assert(cond&& message);
#else
#define MMMASSERT(cond, message)
#endif

template <typename ENUMTYPE>
auto getEnumByStr(const std::unordered_map<std::string_view, ENUMTYPE>& map, std::string_view val) {
  static_assert(static_cast<int>(ENUMTYPE::Invalid) == -1);
  auto iter = map.find(val);
  if (iter != map.cend()) { return iter->second; }
  return ENUMTYPE::Invalid;
}


struct WaitController {
  std::mutex mtx;
  std::condition_variable cv;
  bool isready = false;
};


// for ast
template <class ElementType>
std::string join(std::deque<ElementType> const& vec, std::string delim) {
    return join(fmap(vec, [](auto const& a) { return a->toString(); }), delim);
}

template <class T>
static std::string join(std::deque<std::shared_ptr<T>>& vec, std::string const& delim) {
  return join(fmap(vec, [](auto const& a) { return a->toString(); }), delim);
}


template <typename T, typename... U>
size_t getAddressfromFun(std::function<T(U...)> f) {
  typedef T(fnType)(U...);
  fnType** fnPointer = f.template target<fnType*>();
  return (size_t)*fnPointer;
}

class MIMIUM_DLL_PUBLIC Logger {
 public:
  Logger() {
    setoutput(std::cerr);
    Logger::current_report_level = Logger::DEBUG;
  };
  explicit Logger(std::ostream& out) {
    setoutput(out);
    Logger::current_report_level = Logger::DEBUG;
  };
  virtual ~Logger() = default;
  using REPORT_LEVEL = enum { FATAL = 1, ERROR_, WARNING, INFO, DEBUG, TRACE };
  /// @callgraph
  static void debug_log(const std::string& str, REPORT_LEVEL report_level) {
    if (report_level <= Logger::current_report_level) {
      std::string content = report_str.at(report_level) + ": " + str + norm + "\n";
      *output << content;
    }
  }

  inline static void setoutput(std::ostream& out) { Logger::output = &out; }
  static inline REPORT_LEVEL current_report_level = Logger::WARNING;

 private:
  static inline std::ostream* output = &std::cerr;

  static inline const std::string red = "\033[1;31m";
  static inline const std::string green = "\033[1;32m";

  static inline const std::string yellow = "\033[1;33m";
  static inline const std::string norm = "\033[0m";
  static inline const std::map<Logger::REPORT_LEVEL, std::string> report_str = {
      {FATAL, red + "Fatal"}, {ERROR_, red + "Error"}, {WARNING, yellow + "Warning"},
      {INFO, green + "Info"}, {DEBUG, "Debug"},        {TRACE, "Trace"}};
};

}  // namespace mimium