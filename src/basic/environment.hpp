/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <future>
#include <memory>
#include <unordered_map>
#include <utility>

#include "basic/helper_functions.hpp"
#include "basic/variant_visitor_helper.hpp"
namespace mimium {
struct RenameEnvironment : public std::enable_shared_from_this<RenameEnvironment> {
  std::unordered_map<std::string, std::string> rename_map{};
  std::shared_ptr<RenameEnvironment> parent_env = nullptr;

  std::shared_ptr<RenameEnvironment> expand() {
    auto newenv = std::make_shared<RenameEnvironment>();
    newenv->parent_env = shared_from_this();
    return newenv;
  }
  void addToMap(std::string const& namel, std::string const& namer) {
    rename_map.emplace(namel, namer);
  }
  template <typename T>
  std::optional<T> metaSearch(std::optional<std::string> const& name, std::future<T>&& ret_local,
                              std::future<std::optional<T>>&& ret_freevar) {
    if (name && rename_map.count(name.value()) > 0) { return ret_local.get(); }
    if (name && parent_env != nullptr) { return ret_freevar.get(); }
    return std::nullopt;
  }
#define THUNK(VAL) std::async(std::launch::deferred, [&]() { return VAL; })  // NOLINT

  std::optional<std::string> search(std::optional<std::string> const& name) {
    return metaSearch(name, THUNK(rename_map.at(name.value())), THUNK(parent_env->search(name)));
  }
  std::optional<bool> isFreeVar(std::string const& name) {
    return metaSearch(name, THUNK(false), THUNK(std::optional(true)));
  }
};
}  // namespace mimium