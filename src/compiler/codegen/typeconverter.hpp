/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/type.hpp"
namespace llvm{
  class Type;
  class Module;
  class IRBuilderBase;
}
namespace mimium {

struct TypeConverter {
  explicit TypeConverter(llvm::IRBuilderBase& b, llvm::Module& m)
      : builder(b), module(m), tmpname(""){};
  llvm::IRBuilderBase& builder;
  llvm::Module& module;
  std::string tmpname;
  std::unordered_map<std::string, llvm::Type*> aliasmap;
  static void error() { throw std::runtime_error("Invalid Type"); }

  llvm::Type* operator()(types::None const& i);
  llvm::Type* operator()(types::TypeVar const& i);
  llvm::Type* operator()(types::Void const& i);
  llvm::Type* operator()(types::Float const& i);
  llvm::Type* operator()(types::String const& i);
  llvm::Type* operator()(types::Ref const& i);
  llvm::Type* operator()(types::Pointer const& i);
  llvm::Type* operator()(types::Function const& i);
  llvm::Type* operator()(types::Closure const& i);
  llvm::Type* operator()(types::Array const& i);
  llvm::Type* operator()(types::Struct const& i);
  llvm::Type* operator()(types::Tuple const& i);
  llvm::Type* operator()(types::Alias const& i);

 private:
  [[nodiscard]] std::string consumeAlias();
  llvm::Type* convertTypeArray(std::vector<llvm::Type*>&& tarray);
  [[nodiscard]] std::optional<llvm::Type*> tryGetNamedType(std::string& name) const;
};

}  // namespace mimium
