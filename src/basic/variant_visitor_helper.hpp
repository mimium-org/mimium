#pragma once
#include <variant>

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
  Rec_Wrap(T t_) {
    t.reserve(1);
    t.emplace_back(std::move(t_));
  }  // NOLINT

  // cast back to wrapped type
  operator T&() { return t.front(); }              // NOLINT
  operator const T&() const { return t.front(); }  // NOLINT

  T& getraw() { return t.front(); }
  // store the value
  std::vector<T> t;
};

template <typename T>
inline bool operator==(const Rec_Wrap<T>& t1, const Rec_Wrap<T>& t2) {
  return t1.t.front() == t2.t.front();
}
template <typename T>
inline bool operator!=(const Rec_Wrap<T>& t1, const Rec_Wrap<T>& t2) {
  return !(t1 == t2);
}
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
constexpr  bool holds_alternative(const std::variant<Types...>& v) {
  return std::holds_alternative<Rec_Wrap<T>>(v);
}

template <class T, class... Types>
constexpr  bool holds_alternative(const std::variant<Types...>&& v) {
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