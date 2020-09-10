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
// helper type for visiting

namespace mimium {
template <typename T>
class Environment : public std::enable_shared_from_this<Environment<T>> {
  std::string name;
  std::vector<std::shared_ptr<Environment<T>>> children;
  std::shared_ptr<Environment<T>> parent;
  std::unordered_map<std::string, T> variables;

 public:
  Environment() : name(""), parent(nullptr) {}
  Environment(std::string name_i, std::shared_ptr<Environment<T>> parent_i)
      : name(std::move(name_i)), parent(parent_i) {}
  virtual ~Environment() = default;
  auto findVariable(std::string key) -> T {
    T res;
    if (!variables.empty() && variables.count(key) > 0) {  // search dictionary
      res = variables.at(key);
    } else if (parent != nullptr) {
      res = parent->findVariable(key);  // search recursively
    } else {
      throw std::logic_error("Variable " + key + " not found");
      res = nullptr;
    }
    return res;
  };
  std::pair<bool, bool> isFreeVariable(std::string key) {
    // return value: [isvarset , isfv]
    std::pair<bool, bool> res;
    if (variables.count(key) > 0) {  // search dictionary
      res = std::pair(true, false);  // return false if var is local variable
    } else if (parent != nullptr) {
      auto [isvarset, isisfv] = parent->isFreeVariable(key);
      res = std::pair(isvarset, true);  // search recursively
    } else {
      throw std::logic_error("Variable " + key + " not found");
      res = std::pair(false, false);
    }
    return res;
  };
  bool isVariableSet(std::string key) {
    bool res;
    if (variables.count(key) > 0) {  // search dictionary
      res = true;
    } else if (parent != nullptr) {
      res = parent->isVariableSet(key);  // search recursively
    } else {
      res = false;
    }
    return res;
  }
  virtual void setVariable(std::string key, T val) {
    if (!variables.empty() && variables.count(key) > 0) {  // search dictionary

      Logger::debug_log("overwritten", Logger::DEBUG);
      variables.insert_or_assign(key, val);  // overwrite exsisting value

    } else if (parent != nullptr) {
      parent->setVariable(key, val);  // search recursively
    } else {
      Logger::debug_log("Create New Variable " + key, Logger::DEBUG);
      variables.emplace(key, val);
    }
  };

  void setVariableRaw(std::string key, T val) {
    Logger::debug_log("Create New variable" + key, Logger::DEBUG);
    variables.emplace(key, val);
  };

  auto& getVariables() { return variables; }
  auto getParent() { return parent; }
  bool isRoot() { return parent == nullptr; }
  std::string getName() { return name; };
  auto createNewChild(std::string newname) -> std::shared_ptr<Environment<T>> {
    auto child = std::make_shared<Environment<T>>(newname, this->shared_from_this());
    children.push_back(std::move(child));
    return children.back();
  }
  void deleteLastChild() { children.pop_back(); }
};

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