/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
 
#include "compiler/codegen/typeconverter.hpp"

namespace mimium {

llvm::Type* TypeConverter::operator()(types::None& /*i*/) {
  error();
  return nullptr;
}
llvm::Type* TypeConverter::operator()(types::TypeVar& /*i*/) {
  throw std::logic_error("Type inference failed");
  return nullptr;
}
llvm::Type* TypeConverter::operator()(types::Void& /*i*/) {
  return builder.getVoidTy();
}
llvm::Type* TypeConverter::operator()(types::Float& /*i*/) {
  return builder.getDoubleTy();
}
llvm::Type* TypeConverter::operator()(types::String& /*i*/) {
  return builder.getInt8PtrTy();
}
llvm::Type* TypeConverter::operator()(types::Ref& i) {
  return llvm::PointerType::get(std::visit(*this, i.val), 0);
}
llvm::Type* TypeConverter::operator()(types::Pointer& i) {
  return llvm::PointerType::get(std::visit(*this, i.val), 0);
}
llvm::Type* TypeConverter::operator()(types::Function& i) {
  std::vector<llvm::Type*> ar;
  for (auto& a : i.arg_types) {
    ar.push_back(std::visit(*this, a));
  }
  llvm::Type* ret = std::visit(*this, i.ret_type);
  if (rv::holds_alternative<types::Function>(i.ret_type)) {
    ret = llvm::PointerType::get(ret, 0);
  }
  return llvm::FunctionType::get(ret, ar, false);
}
llvm::Type* TypeConverter::operator()(types::Closure& i) {
  auto name = consumeAlias();
  auto* capturetype = std::visit(*this, i.captures);
  auto* fty = (*this)(i.fun);
  return llvm::StructType::create(builder.getContext(), {fty, capturetype},
                                  name, false);
}
llvm::Type* TypeConverter::operator()(types::Array& i) {
  return llvm::PointerType::get(std::visit(*this, i.elem_type),0);
}
llvm::Type* TypeConverter::operator()(types::Struct& i) {
  std::vector<llvm::Type*> ar;
  llvm::Type* res = nullptr;
  auto t = static_cast<types::Tuple>(i);
  for (auto& a : t.arg_types) {
    ar.push_back(std::visit(*this, a));
  }
  if (tmpname.empty()) {
    res = llvm::StructType::get(builder.getContext(), ar);
  } else {
    auto n = consumeAlias();
    res = tryGetNamedType(n);
    if (res == nullptr) {
      res = llvm::StructType::create(builder.getContext(), ar, n, false);
    }
  }
  return res;
}
llvm::Type* TypeConverter::operator()(types::Tuple& i) {
  std::vector<llvm::Type*> ar;
  llvm::Type* res = nullptr;
  for (auto& a : i.arg_types) {
    ar.push_back(std::visit(*this, a));
  }
  if (tmpname.empty()) {
    res = llvm::StructType::get(builder.getContext(), ar, false);
  } else {
    auto n = consumeAlias();
    res = tryGetNamedType(n);
    if (res == nullptr) {
      res = llvm::StructType::create(builder.getContext(), ar, n, false);
    }
  }
  return res;
}
// llvm::Type* TypeConverter::operator()(types::Time& i) {
//   llvm::Type* res;
//   if (tmpname.empty()) {
//     res = llvm::StructType::get(
//         builder.getContext(),
//         {builder.getDoubleTy(), std::visit(*this, i.val)});
//   } else {
//     auto n = consumeAlias();
//     res = tryGetNamedType(n);
//     if (res == nullptr) {
//       res = llvm::StructType::create(
//           builder.getContext(),
//           {builder.getDoubleTy(), std::visit(*this, i.val)}, n);
//     }
//   }
//   return res;
// }
llvm::Type* TypeConverter::operator()(types::Alias& i) {
  auto it = aliasmap.find(i.name);
  llvm::Type* res = nullptr;
  if (it == aliasmap.end()) {
    tmpname = i.name;
    res = std::visit(*this, i.target);
    aliasmap.emplace(i.name, res);
  } else {
    res = it->second;
  }
  return res;
}

std::string TypeConverter::consumeAlias() {
  std::string t = tmpname;
  tmpname = "";
  return t;
}

llvm::Type* TypeConverter::tryGetNamedType(std::string& name)const {
  return module.getTypeByName(name);
}

}  // namespace mimium