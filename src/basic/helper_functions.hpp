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
#include <numeric>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>  //pair
#include <variant>
#include <vector>

#include "llvm/Support/Error.h"
#include "variant_visitor_helper.hpp"

#ifdef _WIN32
SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE),
               ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

namespace mimium {

//meta function to check if it is smart pointer or not(used in ast_to_string);
template <typename T, typename Enable = void>
struct is_smart_pointer {
  enum { value = false };
};

template <typename T>
struct is_smart_pointer<
    T, typename std::enable_if<std::is_same<
           typename std::remove_cv<T>::type,
           std::shared_ptr<typename T::element_type>>::value>::type> {
  enum { value = true };
};

template <typename T>
struct is_smart_pointer<
    T, typename std::enable_if<std::is_same<
           typename std::remove_cv<T>::type,
           std::unique_ptr<typename T::element_type>>::value>::type> {
  enum { value = true };
};

template <typename T>
struct is_smart_pointer<
    T, typename std::enable_if<std::is_same<
           typename std::remove_cv<T>::type,
           std::weak_ptr<typename T::element_type>>::value>::type> {
  enum { value = true };
};

struct WaitController {
  std::mutex mtx;
  std::condition_variable cv;
  bool isready = false;
};

// for ast
template <class ElementType>
static std::string join(std::deque<ElementType>& vec, std::string delim) {
  return std::accumulate(std::next(vec.begin()), vec.end(),
                         vec.begin()->toString(),
                         [&](std::string a, std::shared_ptr<ElementType>& b) {
                           return std::move(a) + delim + b.toString();
                         });
};

template <class T>
bool has(std::vector<T> t, T s) {
  return std::find(t.begin(), t.end(), s) != t.end();
}
inline bool has(std::vector<std::string> t, char* s) {
  return std::find(t.begin(), t.end(), std::string(s)) != t.end();
}

[[maybe_unused]] static std::string join(std::deque<std::string>& vec,
                                         std::string delim) {
  std::string res;
  if (!vec.empty()) {
    res = std::accumulate(
        std::next(vec.begin()), vec.end(), *(vec.begin()),
        [&](std::string a, std::string b) { return std::move(a) + delim + b; });
  }
  return res;
};
template <class T>
static std::string join(std::deque<std::shared_ptr<T>>& vec,
                        std::string delim) {
  std::string res;
  if (!vec.empty()) {
    res = std::accumulate(std::next(vec.begin()), vec.end(),
                          (*(vec.begin()))->toString(),
                          [&](std::string a, std::shared_ptr<T>& b) {
                            return std::move(a) + delim + b->toString();
                          });
  }
  return res;
};

// static std::string join(const std::vector<TypedVal>& vec, std::string delim)
// {
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
  Logger() {
    setoutput(std::cerr);
    Logger::current_report_level = Logger::DEBUG;
  };
  explicit Logger(std::ostream& out) {
    setoutput(out);
    Logger::current_report_level = Logger::DEBUG;
  };
  virtual ~Logger() = default;
  using REPORT_LEVEL = enum { FATAL = 1, ERROR, WARNING, INFO, DEBUG, TRACE };
  /// @callgraph
  static void debug_log(const std::string& str, REPORT_LEVEL report_level) {
    if (report_level <= Logger::current_report_level) {
      std::string content =
          report_str.at(report_level) + ": " + str + norm + "\n";
      if (report_level <= REPORT_LEVEL::ERROR) {
        throw std::runtime_error(content);
      }
      *output << content;
    }
  }
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
  static inline REPORT_LEVEL current_report_level = Logger::WARNING;

 private:
  static inline std::ostream* output = &std::cerr;

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