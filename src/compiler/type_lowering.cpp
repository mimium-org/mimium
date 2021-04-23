#include "type_lowering.hpp"

namespace mimium {

namespace {
template <class T>
constexpr bool is_aggregate = !std::is_empty_v<std::decay_t<T>>;

bool isAggregate(HType::Value const& t) {
  // stringはchar*に変換されるのでプリミティブだけど変換
  return std::visit([](auto const& a) { return !is_aggregate<decltype(a)>; }, t.v) ||
         std::holds_alternative<HType::String>(t.v);
}

}  // namespace

LType::Value lowerType(HType::Value const& t) {
  auto&& mapper = [](HType::Value const& a) { return Box(lowerType(a)); };
  auto&& vis = overloaded{
      [](HType::Unit const&  /*a*/) { return LType::Value{LType::Unit{}}; },
      [](HType::Bool const&  /*a*/) { return LType::Value{LType::Bool{}}; },
      [](HType::Int const&  /*a*/) { return LType::Value{LType::Int{}}; },
      [](HType::Float const&  /*a*/) { return LType::Value{LType::Float{}}; },
      [](HType::String const&  /*a*/) {
        return LType::Value{LType::Pointer{LType::Value{LType::String{}}}};
      },
      [](HType::Variant const& a) {//TODO
        auto tag = LType::Int{};
        auto val = LType::Pointer{LType::Value{LType::Unit{}}};
        return LType::Value{LType::Tuple{{LType::Value{tag}, LType::Value{val}}}};
      },
      [&](HType::Tuple const& a) { return LType::Value{LType::Tuple{fmap(a.v, mapper)}}; },
      [&](HType::Function const& a) {
        auto arglist = fmap(a.v.first, mapper);
        if (isAggregate(a.v.second)) {
          arglist.push_front(lowerType(a.v.second));
          return LType::Value{LType::Function{std::pair(arglist, LType::Value{LType::Unit{}})}};
        }
        return LType::Value{LType::Function{std::pair(arglist, lowerType(a.v.second))}};
      },
      [](HType::Array const& a) { return LType::Value{LType::Pointer{lowerType(a.v.v)}}; },
      [](HType::Record const& a) {
        return LType::Value{
            LType::Tuple{fmap(a.v, [](HType::RecordKey const& a) { return Box(lowerType(a.v)); })}};
      },
      [](HType::Identified const& a) { return lowerType(a.v.v); },
      [](HType::Alias const& a) {
        return LType::Value{LType::Alias{a.v.id, lowerType(a.v.v)}};
      },
      [](HType::ListT const& a) {
        auto elemtype = lowerType(a.v);
        return LType::Value{LType::Tuple{{elemtype, LType::Value{LType::Pointer{elemtype}}}}};
      }};
      return std::visit(vis,t.v);
}

}  // namespace mimium