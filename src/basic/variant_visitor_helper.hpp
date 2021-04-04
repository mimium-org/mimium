/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <cassert>
#include <memory>
#include <variant>

template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace mimium {
// recursive variant

template <typename T>
struct Box {
  // construct from an existing object
  Box() = delete;
  Box(T rt) : t(std::make_shared<T>(std::move(rt))) {}  // NOLINT
  template <class U>
  using enabler = std::enable_if_t<std::is_same_v<std::decay_t<U>, T>>;
  template <typename U, enabler<U>>
  Box(U&& rt) : t(std::make_shared<U>(std::forward<U>(rt))) {}  // NOLINT
  // cast back to wrapped type
  operator T&() { return *t; }              // NOLINT
  operator const T&() const { return *t; }  // NOLINT

  T& getraw() const { return *t; }
  // store the value
  std::shared_ptr<T> t;
};

template <typename T>
bool operator==(const Box<T>& t1, const Box<T>& t2) {
  return static_cast<const T&>(t1) == static_cast<const T&>(t2);
}
template <typename T>
bool operator!=(const Box<T>& t1, const Box<T>& t2) {
  return !(t1 == t2);
}


namespace rv {

template <class T, class VARIANT>
constexpr bool holds_alternative(VARIANT&& v) {
  return std::holds_alternative<Box<T>>(std::forward<decltype(v)>(v));
}

template <class T, class VARIANT>
constexpr auto get(VARIANT&& v) -> decltype(auto) {
  return std::get<Box<T>>(std::forward<decltype(v)>(v)).getraw();
}

}  // namespace rv
}  // namespace mimium