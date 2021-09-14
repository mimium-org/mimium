/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/type_lowering.hpp"  //for isAggregate Function

#include "compiler/codegen/llvm_header.hpp"
#include "compiler/codegen/typeconverter.hpp"

namespace mimium {

// llvm::Type* TypeConverter::operator()(LType::None const& /*i*/) {
//   error();
//   return nullptr;
// }
// llvm::Type* TypeConverter::operator()(LType::TypeVar const& /*i*/) {
//   throw std::runtime_error("Type inference failed");
//   return nullptr;
// }
llvm::Type* TypeConverter::operator()(LType::Unit const& /*i*/) { return builder.getVoidTy(); }
llvm::Type* TypeConverter::operator()(LType::Bool const& /*i*/) { return builder.getInt1Ty(); }
llvm::Type* TypeConverter::operator()(LType::Int const& /*i*/) { return builder.getInt32Ty(); }
llvm::Type* TypeConverter::operator()(LType::Float const& /*i*/) { return builder.getDoubleTy(); }
llvm::Type* TypeConverter::operator()(LType::String const& /*i*/) { return builder.getInt8PtrTy(); }
// llvm::Type* TypeConverter::operator()(LType::Ref const& i) {
//   auto* elemty = convertType( i.val);
//   if (elemty->isVoidTy()) { elemty = builder.getInt8Ty(); }
//   return llvm::PointerType::get(elemty, 0);
// }
llvm::Type* TypeConverter::operator()(LType::Pointer const& i) {
  return llvm::PointerType::get(convertType(i.v), 0);
}
llvm::Type* TypeConverter::operator()(LType::Function const& i) {
  std::vector<llvm::Type*> ar;
  llvm::Type* ret = convertType(i.v.second);
  if (LType::canonicalCheck<LType::Function>(i.v.second.getraw())) {
    ret = llvm::PointerType::get(ret, 0);
  }
  for (auto a : i.v.first) {
    if (LType::canonicalCheck<LType::Function>(a.getraw())) { a = LType::Value{LType::Pointer{a}}; }
    ar.push_back(convertType(a));
  }
  return llvm::FunctionType::get(ret, ar, false);
}
// llvm::Type* TypeConverter::operator()(LType::Closure const& i) {
//   auto name = consumeAlias();
//   auto* capturetype = convertType( i.captures);
//   auto* fty = (*this)(i.fun);
//   return llvm::StructType::create(builder.getContext(), {fty, capturetype}, name, false);
// }
llvm::Type* TypeConverter::operator()(LType::Array const& i) {
  // note that zero-sized array can be automatically interpreted as variable-sized array
  return llvm::ArrayType::get(convertType(i.v.v), i.v.size);
}

llvm::Type* TypeConverter::convertTypeArray(std::vector<llvm::Type*>&& tarray) {
  if (tmpname.empty()) { return llvm::StructType::get(builder.getContext(), tarray); }
  auto n = consumeAlias();
  

  return tryGetNamedType(n).value_or(
      llvm::StructType::get(builder.getContext(),tarray)
      //  llvm::StructType::create(builder.getContext(), n , tarray,false)
      );
}

llvm::Type* TypeConverter::operator()(LType::Record const& i) {
  return convertTypeArray(fmap<List, std::vector>(
      i.v, [&](LType::RecordCategory const& a) -> llvm::Type* { return convertType(a.v); }));
}
llvm::Type* TypeConverter::operator()(LType::Tuple const& i) {
  return convertTypeArray(fmap<List, std::vector>(
      i.v, [&](const LType::Value& a) -> llvm::Type* { return convertType(a); }));
}
// llvm::Type* TypeConverter::operator()(LType::Time const& i) {
//   llvm::Type* res;
//   if (tmpname.empty()) {
//     res = llvm::StructType::get(
//         builder.getContext(),
//         {builder.getDoubleTy(), convertType( i.val)});
//   } else {
//     auto n = consumeAlias();
//     res = tryGetNamedType(n);
//     if (res == nullptr) {
//       res = llvm::StructType::create(
//           builder.getContext(),
//           {builder.getDoubleTy(), convertType( i.val)}, n);
//     }
//   }
//   return res;
// }
llvm::Type* TypeConverter::operator()(LType::Alias const& i) {
  auto it = aliasmap.find(i.v.id);
  llvm::Type* res = nullptr;
  if (it == aliasmap.end()) {
    tmpname = i.v.id;
    res = convertType(i.v.v);
    assert(tmpname.empty());
    aliasmap.emplace(i.v.id, res);
    return res;
  }
  return it->second;
}
llvm::Type* TypeConverter::operator()(LType::Identified const& i) { return convertType(i.v.v); }

std::string TypeConverter::consumeAlias() {
  std::string t = tmpname;
  tmpname = "";
  return t;

}

std::optional<llvm::Type*> TypeConverter::tryGetNamedType(std::string const& name) const {
  auto* res = module.getTypeByName(name);
  return (res != nullptr) ? std::optional(res) : std::nullopt;
}

}  // namespace mimium