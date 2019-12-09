#pragma once
#include <memory>
#include <unordered_map>
#include <utility>

#include "ast.hpp"
#include "helper_functions.hpp"
#include "variant_visitor_helper.hpp"
// helper type for visiting

using mClosure_ptr = std::shared_ptr<mimium::Closure>;
using mValue = std::variant<double, std::shared_ptr<AST>, mClosure_ptr,
                            std::vector<double>, std::string>;

namespace mimium {
template <typename T>
class Environment : public std::enable_shared_from_this<Environment<T>> {
  std::vector<std::shared_ptr<Environment<T>>> children;
  std::string name;

  protected:
    std::shared_ptr<Environment<T>> parent;
    std::unordered_map<std::string, T> variables;

 public:
  Environment() : parent(nullptr), name("") {}
  Environment(std::string name_i, std::shared_ptr<Environment<T>> parent_i)
      : name(std::move(name_i),parent(std::move(parent_i))) {}
  auto findVariable(std::string key)->T {
    T res;
    if (!variables.empty() && variables.count(key) > 0) {  // search dictionary
      res = variables.at(key);
    } else if (parent != nullptr) {
      res = parent->findVariable(key);  // search recursively
    } else {
      throw std::runtime_error("Variable " + key + " not found");
      res = nullptr;
    }
    return res;
  };
  std::pair<bool, bool> isFreeVariable(std::string key) {
    // return value: [isvarset , isfv]
    std::pair<bool, bool> res;
    if (!variables.empty()) {        // search dictionary
      res = std::pair(true, false);  // return false if var is local variable
    } else if (parent != nullptr) {
      auto [isvarset, isisfv] = parent->isFreeVariable(key);
      res = std::pair(isvarset, true);  // search recursively
    } else {
      throw std::runtime_error("Variable " + key + " not found");
      res = std::pair(false, false);
    }
    return res;
  };
  bool isVariableSet(std::string key) {
    bool res;
    if (!variables.empty()) {  // search dictionary
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
  std::string getName() { return name; };
  auto createNewChild(std::string newname) -> std::shared_ptr<Environment<T>> {
    auto child = std::make_shared<Environment<T>>(newname, this->shared_from_this());
    children.push_back(std::move(child));
    return children.back();
  }
  void deleteLastChild() { children.pop_back(); }
  static std::string to_string(mValue m);
};

class InterpreterEnv : public Environment<mValue>{
  public:
    void setVariable(std::string key, mValue val) override;
};
};  // namespace mimium