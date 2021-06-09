#pragma once
#include "basic/type_new.hpp"

namespace mimium {
template <class T>
constexpr bool is_aggregate = !std::is_empty_v<std::decay_t<T>>;

template <class TCLASS>
bool isAggregate(typename TCLASS::Value const& t) {
  // stringはchar*に変換されるのでプリミティブだけど変換
  return std::visit([](auto const& a) { return !is_aggregate<decltype(a)>; }, t.v) ||
         std::holds_alternative<typename TCLASS::String>(t.v);
}
LType::Value lowerType(IType::Value const& t);


LType::Value lowerType(HType::Value const& t);
}