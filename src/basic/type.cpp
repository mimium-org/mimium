/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "basic/type.hpp"

namespace mimium {
namespace types {

bool isTypeVar(types::Value t) { return std::holds_alternative<Box<types::TypeVar>>(t); }

std::string toString(const Value& v, bool verbose) {
  tostrvisitor.verbose = verbose;
  return std::visit(tostrvisitor, v);
}

void dump(const Value& v, bool verbose) { std::cerr << toString(v, verbose) << "\n"; }

}  // namespace types

void TypeEnv::replaceTypeVars() {
  for (auto& [key, val] : env) {
    if (rv::holds_alternative<types::TypeVar>(val)) {
      auto& tv = rv::get<types::TypeVar>(val);
      if (std::holds_alternative<types::None>(tv.contained)) {
        // throw std::runtime_error("type inference for " + key + " failed");
        tv.contained = types::Float{};
      }
      env[key] = tv.contained;
    }
  }
}

std::string TypeEnv::toString(bool verbose) {
  std::stringstream ss;
  types::ToStringVisitor vis;
  vis.verbose = verbose;
  for (auto& [key, val] : env) { ss << key << " : " << std::visit(vis, val) << "\n"; }
  return ss.str();
}
void TypeEnv::dump(bool verbose) {
  std::cerr << "-------------------\n" << toString(verbose) << "-------------------\n";
}
void TypeEnv::dumpTvLinks() {
  std::cerr << "------tvlinks-----\n";
  int i = 0;
  for (auto& a : tv_container) {
    std::cerr << "typevar" << i << " : " << types::toString(a) << "\n";
    ++i;
  }
  std::cerr << "------tvlinks-----" << std::endl;
}

}  // namespace mimium