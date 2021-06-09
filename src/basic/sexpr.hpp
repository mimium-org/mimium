#pragma once
#include "abstractions.hpp"
namespace mimium {

template <class S, class SymbolT>
auto makeSExpr(SymbolT const& s);

struct SExpr {
  std::string x;
  std::optional<Box<SExpr>> xs;
};

inline auto cons(std::string const& s, SExpr const& s2) {
  auto res = SExpr{s, std::nullopt};
  res.xs = std::optional(s2);
  return res;
}
inline std::optional<Box<SExpr>>* getEnd(SExpr s) {
  auto* xs = &s.xs;
  while (xs->has_value()) { xs = &xs->value().getraw().xs; }
  return xs;
}

inline auto cons(SExpr s1, SExpr s2) {
  auto* s1end = getEnd(s1);
  *s1end = std::optional(s2);
  return s1;
}

inline auto makeSExpr(std::string s) { return SExpr{s, std::nullopt}; }

template <class iter>
std::optional<Box<SExpr>> makeSExpr(iter&& x, iter&& end);

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

inline auto toString(std::string const& s) { return s; }

inline std::string toString(SExpr const& s) {
  std::string res("(");
  res += toString(s.x);
  if (auto t = tail(s)) { res += toString(t.value()); }
  res += ")";
  return res;
}

}  // namespace mimium