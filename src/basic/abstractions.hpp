/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <list>
#include <numeric>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include "variant_visitor_helper.hpp"
namespace mimium {

template <template <class...> class Category, int ID = 0, class... T>
struct CategoryWrapped {
  using type = Category<T...>;
  constexpr CategoryWrapped() = default;
  type v;
};
template <template <class...> class Category, int ID, class F, class... T>
auto fmap(CategoryWrapped<Category, ID, T...> const& t, F&& lambda) {
  return fmap(t.v, std::forward<F>(lambda));
}

template <class T>
using IdentCategory = T;
template <class T, class F>
auto fmap(IdentCategory<T> const& t, F&& lambda) -> decltype(auto) {
  return std::forward<F>(lambda)(t.v);
}
// template <class T, class F>
// auto fmap(IdentCategory<T>& t, F&& lambda) -> decltype(auto) {
//   return std::forward<F>(lambda)(t.v);
// }

template <class T, class F>
auto fmap(std::optional<T> const& v, F&& lambda) -> decltype(auto) {
  return v ? std::optional(std::forward<F>(lambda)(v.value())) : std::nullopt;
}

template <class T>
using List = std::list<T>;

template <class T>
using Set = std::unordered_set<T>;
template <class FROM, class TO>
using Map = std::unordered_map<FROM, TO>;

template <class T, class U = T>
using Pair = std::pair<T, U>;
template <class T, class F>
auto fmap(Pair<T> const& v, F&& lambda) -> decltype(auto) {
  return Pair<T>(std::forward<decltype(lambda)>(lambda(v.first)),
                 std::forward<decltype(lambda)>(lambda(v.second)));
}
template <class T, class F>
auto foldl(Pair<T> const& v, F&& lambda) {
  return std::forward<decltype(lambda)>(v.first, v.second);
}

template <class R, class... ArgTypes>
using Fn = std::function<R(ArgTypes...)>;

template <class T, class U>
using Either = std::variant<T, U>;

template <class T, template <class...> class C, template <class...> class... Cs>
struct Nested {
  using type = C<Nested<T, Cs...>>;
};

template <template <class...> class CONTAINERIN,
          template <class...> class CONTAINEROUT = CONTAINERIN, typename ELEMENTIN, typename LAMBDA>
auto fmap(CONTAINERIN<ELEMENTIN> const& args, LAMBDA&& lambda)
    -> CONTAINEROUT<decltype(lambda(*args.begin()))> {
  static_assert(std::is_invocable_v<LAMBDA, ELEMENTIN>, "the function for fmap is not invocable");
  CONTAINEROUT<decltype(lambda(*args.begin()))> res;
  std::transform(args.cbegin(), args.cend(), std::back_inserter(res),
                 std::forward<decltype(lambda)>(lambda));
  return std::move(res);
}
template <template <class...> class CONTAINERIN,
          template <class...> class CONTAINEROUT = CONTAINERIN, typename ELEMENTIN, typename LAMBDA>
auto fmap(CONTAINERIN<ELEMENTIN>& args, LAMBDA&& lambda)
    -> CONTAINEROUT<decltype(lambda(*args.begin()))> {
  static_assert(std::is_invocable_v<LAMBDA, ELEMENTIN>, "the function for fmap is not invocable");
  CONTAINEROUT<decltype(lambda(*args.begin()))> res;
  std::transform(args.begin(), args.end(), std::back_inserter(res),
                 std::forward<decltype(lambda)>(lambda));
  return std::move(res);
}

template <template <class...> class CONTAINER, typename RES, typename LAMBDA>
auto foldl(CONTAINER<RES> const& input, LAMBDA&& lambda) {
  return std::accumulate(std::cbegin(input), std::cend(input), RES{},
                         std::forward<decltype(lambda)>(lambda));
}

template <template <class...> class CONTAINER, typename RES, typename LAMBDA>
auto foldl(CONTAINER<RES> & input, LAMBDA&& lambda) {
  return std::accumulate(std::begin(input), std::end(input), RES{},
                         std::forward<decltype(lambda)>(lambda));
}

template <template <class...> class Container>
std::string join(Container<std::string> const& vec, std::string const& delim) {
  return foldl(vec, [&](const auto& a, const auto& b) { return a + delim + b; });
}

template <template <class...> class Container, class T>
bool has(Container<T> const& t, T&& s) {
  return std::find(t.cbegin(), t.ecnd(), std::forward<T>(s)) != t.cend();
}

}  // namespace mimium