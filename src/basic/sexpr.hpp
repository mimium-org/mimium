#pragma once
#include "abstractions.hpp"
namespace mimium {

template <class S, class SymbolT>
auto makeSExpr(SymbolT const& s);

struct SExpr {
  using Leaf = std::variant<Box<SExpr>, std::string>;
  Leaf x;
  std::optional<Leaf> xs;
};

inline auto cons(std::string const& s, SExpr const& s2) {
  return SExpr{s, std::optional(s2)};
  ;
}
inline auto cons(SExpr s1, SExpr s2) { return SExpr{s1, std::optional(s2)}; }

inline auto makeSExpr(std::string s) { return SExpr{s, std::nullopt}; }

template <class iter>
std::optional<Box<SExpr>> makeSExpr(iter&& x, iter&& end) {
  std::optional<Box<SExpr>> s = std::nullopt;
  if (x != end) {
    s = std::optional(SExpr{
        *x, makeSExpr(std::next(std::forward<decltype(x)>(x)), std::forward<decltype(end)>(end))});
  }
  return s;
}

inline auto makeSExpr(std::initializer_list<std::string> list) {
  return makeSExpr(list.begin(), list.end()).value().getraw();
}

inline auto& head(SExpr const& s) { return s.x; }
inline auto& tail(SExpr const& s) { return s.xs; }
inline std::string toString(SExpr const& s);
inline std::string toString(SExpr::Leaf const& leaf) {
  return std::visit(overloaded{[](std::string const& s) { return s; },
                               [](Box<SExpr> const& s) { return toString(s.getraw()); }},
                    leaf);
}

inline std::string toString(SExpr const& s) {
  if (s.xs.has_value()) {
    std::string tail_s = toString(s.xs.value());
    return "(" + toString(head(s)) + " " + tail_s + ")";
  }
  return toString(head(s));
}

}  // namespace mimium