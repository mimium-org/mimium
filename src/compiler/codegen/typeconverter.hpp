/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/type.hpp"
#include "compiler/codegen/llvm_header.hpp"

namespace mimium {
struct TypeConverter {
  explicit TypeConverter(llvm::IRBuilder<>& b, llvm::Module& m)
      : builder(b), module(m), tmpname(""){};
  llvm::IRBuilder<>& builder;
  llvm::Module& module;
  std::string tmpname;
  std::unordered_map<std::string, llvm::Type*> aliasmap;
  static void error() { throw std::logic_error("Invalid Type"); }

  llvm::Type* operator()(types::None& i);
  llvm::Type* operator()(types::TypeVar& i);
  llvm::Type* operator()(types::Void& i);
  llvm::Type* operator()(types::Float& i);
  llvm::Type* operator()(types::String& i);
  llvm::Type* operator()(types::Ref& i);
  llvm::Type* operator()(types::Pointer& i);
  llvm::Type* operator()(types::Function& i);
  llvm::Type* operator()(types::Closure& i);
  llvm::Type* operator()(types::Array& i);
  llvm::Type* operator()(types::Struct& i);
  llvm::Type* operator()(types::Tuple& i);
  // llvm::Type* operator()(types::Time& i);
  llvm::Type* operator()(types::Alias& i);

 private:
  [[nodiscard]]std::string consumeAlias();
  [[nodiscard]]llvm::Type* tryGetNamedType(std::string& name);
};

}  // namespace mimium
