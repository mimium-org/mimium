/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/helper_functions.hpp"
namespace mimium {

template <class From, class To>
struct Environment : public std::enable_shared_from_this<Environment<From, To>> {
  Map<From, To> map{};
  using EnvPtr = std::shared_ptr<Environment>;
  std::optional<EnvPtr> parent_env = std::nullopt;

  EnvPtr expand() {
    auto newenv = std::make_shared<Environment<From, To>>();
    newenv->parent_env = this->shared_from_this();
    return newenv;
  }
  void addToMap(From const& namel, To const& namer) { map.emplace(namel, namer); }

#define THUNK(VAL) [&]() { return VAL; }  // NOLINT

  std::optional<To> search(From const& name) {
    return metaSearch<To>(name, THUNK(map.at(name)), THUNK(parent_env.value()->search(name)));
  }
  std::optional<bool> isFreeVar(From const& name) {
    return metaSearch<bool>(name, THUNK(false), THUNK(std::optional(true)));
  }
  bool existInLocal(From const& name) {
    auto opt_isfreevar = isFreeVar(name);
    return !(opt_isfreevar.value_or(true));
  }

 private:
  template <typename T, typename Lambda1, typename Lambda2>
  std::optional<T> metaSearch(From const& name, Lambda1&& cont_local, Lambda2&& cont_freevar) {
    if (map.count(name) > 0) { return std::forward<decltype(cont_local)>(cont_local)(); }
    if (parent_env.has_value()) { return std::forward<decltype(cont_freevar)>(cont_freevar)(); }
    return std::nullopt;
  }
};
using RenameEnvironment = Environment<std::string, std::string>;
}  // namespace mimium