/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#define LLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING 1
#include <initializer_list>
#include <unordered_map>

// #include "compiler/runtime/mididriver.hpp"
#include "basic/type.hpp"
#include "sndfile.h"

namespace mimium {

struct BuiltinFnInfo {
  types::Value mmmtype;
  std::string target_fnname;
};

inline BuiltinFnInfo initBI(types::Function&& f,std::string&&s){
  return BuiltinFnInfo{std::move(f),std::move(s)};
}

struct LLVMBuiltin {
  static std::unordered_map<std::string, BuiltinFnInfo> ftable;
  static bool isBuiltin(std::string fname) {
    return LLVMBuiltin::ftable.count(fname) > 0;
  }
};

}  // namespace mimium