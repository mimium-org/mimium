#pragma once
#define LLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING 1
#include <initializer_list>
#include <unordered_map>

// #include "compiler/runtime/mididriver.hpp"
#include "basic/type.hpp"

namespace mimium {

struct BuiltinFnInfo {
  BuiltinFnInfo() = default;
  BuiltinFnInfo(const BuiltinFnInfo& f) = default;
  BuiltinFnInfo(BuiltinFnInfo&& f) = default;
  ~BuiltinFnInfo() = default;
  BuiltinFnInfo& operator=(const BuiltinFnInfo& b1) = default;
  BuiltinFnInfo& operator=(BuiltinFnInfo&& b1) = default;
  BuiltinFnInfo(types::Function f, std::string s)
      : mmmtype(std::move(f)), target_fnname(std::move(s)) {}
  types::Value mmmtype;
  std::string target_fnname;
};
struct LLVMBuiltin {
  static std::unordered_map<std::string, BuiltinFnInfo> ftable;
  static bool isBuiltin(std::string fname) {
    return LLVMBuiltin::ftable.count(fname) > 0;
  }
};

}  // namespace mimium