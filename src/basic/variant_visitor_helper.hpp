/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <cassert>
#include <variant>
#include <memory>
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...)->overloaded<Ts...>;
namespace mimium{
// recursive variant
template <typename T>
struct Box {
  // construct from an existing object
  Box()=delete;
  Box(T& rt) {//NOLINT: do not mark as explicit! need to construct variant directly through box
    t=std::make_shared<T>(std::move(rt));
  } 
  Box(T&& rt) { //NOLINT
    t=std::make_shared<T>(std::forward<T>(rt));
  } 
// cast back to wrapped type
  operator T&() { return *t; }              // NOLINT
  operator const T&() const { return *t; }  // NOLINT

  T& getraw()const { return *t; }
  // store the value
  std::shared_ptr<T> t;
};

template <typename T>
inline bool operator==(const Box<T>& t1, const Box<T>& t2) {
  return static_cast<const T&>(t1) == static_cast<const T&>(t2);
}
template <typename T>
inline bool operator!=(const Box<T>& t1, const Box<T>& t2) {
  return !(t1 == t2);
}

template <typename RETTYPE>
class VisitorBase {
  public:
  template <typename T>
  RETTYPE operator()(Box<T>& ast) {
    // default action
    return (*this)(static_cast<T&>(ast));
  }
  // in case missing to list variant
  template <typename T>
  RETTYPE operator()(T& /*ast*/) {
    assert(false);
    return RETTYPE{};
  }
};


namespace rv {

// template <class Fun, typename T>
// static auto visit(Fun f, T target) {
//   return std::visit(f, target);
// }
// template <class Fun, typename T>
// static auto visit(Fun f, Box<T> target) {
//   return std::visit(f, static_cast<T>(target));
// }

template <class T, class... Types>
constexpr bool holds_alternative(std::variant<Types...>& v) {
  return std::holds_alternative<Box<T>>(v);
}

template <class T, class... Types>
constexpr bool holds_alternative(std::variant<Types...>&& v) {
  return std::holds_alternative<Box<T>>(v);
}
template <class T, class... Types>
constexpr bool holds_alternative(const std::variant<Types...>& v) {
  return std::holds_alternative<Box<T>>(v);
}

template <class T, class... Types>
constexpr bool holds_alternative(const std::variant<Types...>&& v) {
  return std::holds_alternative<Box<T>>(v);
}

template <class T, class... Types>
constexpr T& get(std::variant<Types...>& v) {
  return static_cast<T&>(std::get<Box<T>>(v));
}

template <class T, class... Types>
constexpr T&& get(std::variant<Types...>&& v) {
  return static_cast<T&&>(std::get<Box<T>>(v));
}
template <class T, class... Types>
constexpr const T& get(const std::variant<Types...>& v) {
  return static_cast<const T&>(std::get<Box<T>>(v));
}

template <class T, class... Types>
constexpr const T&& get(const std::variant<Types...>&& v) {
  return static_cast<const T&&>(std::get<Box<T>>(v));
}
}  // namespace rv
}