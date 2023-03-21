#include "type_lowering.hpp"

namespace mimium {

namespace {


auto&& mapper = [](auto const& a) { return Box(lowerType(a)); };

template <class T>
const auto&& common_visitor = overloaded{
    [](typename T::Unit const& /*a*/) { return LType::Value{LType::Unit{}}; },
    [](typename T::Bool const& /*a*/) { return LType::Value{LType::Bool{}}; },
    [](typename T::Int const& /*a*/) { return LType::Value{LType::Int{}}; },
    [](typename T::Float const& /*a*/) { return LType::Value{LType::Float{}}; },
    [](typename T::String const& /*a*/) {
      return LType::Value{LType::Pointer{LType::Value{LType::String{}}}};
    },
    [](typename T::Variant const& a) {  // TODO
      auto tag = LType::Int{};
      auto val = LType::Pointer{LType::Value{LType::Unit{}}};
      return LType::Value{LType::Tuple{{LType::Value{tag}, LType::Value{val}}}};
    },
    [](typename T::Tuple const& a) {
      return LType::Value{LType::Pointer{LType::Value{LType::Tuple{fmap(a.v, mapper)}}}};
    },
    [](typename T::Function const& a) {
      auto arglist = fmap(a.v.first, mapper);
      if (isAggregate<T>(a.v.second)) {
        arglist.push_front(lowerType(a.v.second));
        return LType::Value{LType::Function{std::pair(arglist, LType::Value{LType::Unit{}})}};
      }
      return LType::Value{LType::Function{std::pair(arglist, lowerType(a.v.second))}};
    },
    [](typename T::Array const& a) { return LType::Value{LType::Pointer{lowerType(a.v.v)}}; },
    [](typename T::Record const& a) {
      return LType::Value{LType::Pointer{LType::Value{
          LType::Record{fmap(a.v, [](typename T::RecordKey const& a) { 
            return LType::RecordCategory{a.key,lowerType(a.v)}; })}}}};
    },
    [](typename T::Identified const& a) { return lowerType(a.v.v); },
    [](typename T::Alias const& a) {
      return LType::Value{LType::Alias{a.v.id, lowerType(a.v.v)}};
    },
    [](typename T::ListT const& a) {
      auto elemtype = lowerType(a.v);
      return LType::Value{LType::Tuple{{elemtype, LType::Value{LType::Pointer{elemtype}}}}};
    }};

template <class... Ts>
struct ITypeVisitor : Ts... {
  using Ts::operator()...;
};

template <class... Ts>
ITypeVisitor(Ts... ts) -> ITypeVisitor<Ts...>;

}  // namespace

LType::Value lowerType(IType::Value const& t) {
  return std::visit(overloaded{common_visitor<IType>,
                               [](IType::Intermediate const& /*unused*/) {
                                 assert(false);
                                 return LType::Value{LType::Unit{}};
                               },
                               [](IType::TypeScheme const& /*unused*/) {
                                 assert(false);
                                 return LType::Value{LType::Unit{}};
                               },
                               [](IType::Unknown const& /*unused*/) {
                                 assert(false);
                                 return LType::Value{LType::Unit{}};
                               }},
                    t.v);
}

LType::Value lowerType(HType::Value const& t) { return std::visit(common_visitor<HType>, t.v); }

}  // namespace mimium