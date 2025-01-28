/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/type_new.hpp"
namespace llvm {
class Type;
class Module;
class IRBuilderBase;
}  // namespace llvm
namespace mimium {

struct TypeConverter {
  explicit TypeConverter(llvm::IRBuilderBase& b, llvm::Module& m)
      : builder(b), module(m), tmpname(""){};
  llvm::IRBuilderBase& builder;
  llvm::Module& module;
  std::string tmpname;
  std::unordered_map<std::string, llvm::Type*> aliasmap;
  static void error() { throw std::runtime_error("Invalid Type"); }
  // Unit, Bool, Int, Float, String, Tuple, Record, Function, Array,
  // Identified, Pointer, Alias
  // llvm::Type* operator()(LType::None const& i);
  // llvm::Type* operator()(LType::TypeVar const& i);
  llvm::Type* operator()(LType::Unit const& i);
  llvm::Type* operator()(LType::Bool const& i);
  llvm::Type* operator()(LType::Int const& i);
  llvm::Type* operator()(LType::Float const& i);
  llvm::Type* operator()(LType::String const& i);
  // llvm::Type* operator()(LType::Ref const& i);
  llvm::Type* operator()(LType::Tuple const& i);
  llvm::Type* operator()(LType::Record const& i);
  llvm::Type* operator()(LType::Function const& i);
  // llvm::Type* operator()(LType::Closure const& i);
  llvm::Type* operator()(LType::Array const& i);
  llvm::Type* operator()(LType::Identified const& i);
  llvm::Type* operator()(LType::Pointer const& i);

  llvm::Type* operator()(LType::Alias const& i);
  template <typename T>
  llvm::Type* operator()(Box<T> const& i) {
    return std::visit(i.getraw());
  }

  auto convertType(LType::Value const& v) { return std::visit(*this, v.v); }
 private:
  [[nodiscard]] std::string consumeAlias();
  llvm::Type* convertTypeArray(std::vector<llvm::Type*>&& tarray);
  [[nodiscard]] std::optional<llvm::Type*> tryGetNamedType(std::string const& name) const;
};

}  // namespace mimium
