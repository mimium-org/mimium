/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#define LLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING 1
#include <initializer_list>
#include <unordered_map>
#include "export.hpp"

// #include "compiler/runtime/mididriver.hpp"
#include "basic/type_new.hpp"

namespace mimium {

struct BuiltinFnInfo {
  constexpr BuiltinFnInfo() = default;
  HType::Value mmmtype;
  std::string_view target_fnname;
};

struct MIMIUM_DLL_PUBLIC Intrinsic {
  const static std::unordered_map<std::string_view, BuiltinFnInfo> ftable;
  static bool isBuiltin(std::string fname) { return Intrinsic::ftable.count(fname) > 0; }
  constexpr static int fixed_delaysize = 48000;
};

}  // namespace mimium