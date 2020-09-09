/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <variant>
#include <memory>
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...)->overloaded<Ts...>;

// recursive variant
template <typename T>
struct Rec_Wrap {
  // construct from an existing object

  Rec_Wrap(T& t_) {
    t=std::make_shared<T>(std::move(t_));
  } 
  Rec_Wrap(T&& t_) { 
    t=std::make_shared<T>(std::forward<T>(t_));
  } 
// cast back to wrapped type
  operator T&() { return *t; }              // NOLINT
  operator const T&() const { return *t; }  // NOLINT

  T& getraw()const { return *t; }
  // store the value
  std::shared_ptr<T> t;
};

template <typename T>
inline bool operator==(const Rec_Wrap<T>& t1, const Rec_Wrap<T>& t2) {
  return t1.getraw() == t2.getraw();
}
template <typename T>
inline bool operator!=(const Rec_Wrap<T>& t1, const Rec_Wrap<T>& t2) {
  return !(t1 == t2);
}

template <typename RETTYPE>
class VisitorBase {
  public:
  template <typename T>
  RETTYPE operator()(Rec_Wrap<T>& ast) {
    // default action
    return (*this)(ast.getraw());
  }
  // in case missing to list asts
  template <typename T>
  RETTYPE operator()(T& /*ast*/) {
    static_assert(true, "missing some visitor functions for ExprVisitor");
  }
};


namespace rv {

// template <class Fun, typename T>
// static auto visit(Fun f, T target) {
//   return std::visit(f, target);
// }
// template <class Fun, typename T>
// static auto visit(Fun f, Rec_Wrap<T> target) {
//   return std::visit(f, static_cast<T>(target));
// }

template <class T, class... Types>
constexpr bool holds_alternative(std::variant<Types...>& v) {
  return std::holds_alternative<Rec_Wrap<T>>(v);
}

template <class T, class... Types>
constexpr bool holds_alternative(std::variant<Types...>&& v) {
  return std::holds_alternative<Rec_Wrap<T>>(v);
}
template <class T, class... Types>
constexpr bool holds_alternative(const std::variant<Types...>& v) {
  return std::holds_alternative<Rec_Wrap<T>>(v);
}

template <class T, class... Types>
constexpr bool holds_alternative(const std::variant<Types...>&& v) {
  return std::holds_alternative<Rec_Wrap<T>>(v);
}

template <class T, class... Types>
constexpr T& get(std::variant<Types...>& v) {
  return static_cast<T&>(std::get<Rec_Wrap<T>>(v));
}

template <class T, class... Types>
constexpr T&& get(std::variant<Types...>&& v) {
  return static_cast<T&&>(std::get<Rec_Wrap<T>>(v));
}
template <class T, class... Types>
constexpr const T& get(const std::variant<Types...>& v) {
  return static_cast<const T&>(std::get<Rec_Wrap<T>>(v));
}

template <class T, class... Types>
constexpr const T&& get(const std::variant<Types...>&& v) {
  return static_cast<const T&&>(std::get<Rec_Wrap<T>>(v));
}
}  // namespace rv