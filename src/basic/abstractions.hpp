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
template <template <class> class C>
struct MakeRecursive {
  using type = C<Box<MakeRecursive>>;
};

template <template <class...> class Category, class T, int ID = 0>
struct CategoryWrapped {
  using type = Category<T>;
  type v;
};

template <class T>
using IdentCategory = T;

template <class T>
using List = std::list<T>;

template <class T>
using Set = std::unordered_set<T>;
template <class FROM, class TO>
using Map = std::unordered_map<FROM, TO>;

template <class T, class U>
using Pair = std::pair<T, U>;

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
CONTAINEROUT<std::invoke_result_t<LAMBDA, ELEMENTIN>> fmap(CONTAINERIN<ELEMENTIN> const& args,
                                                           LAMBDA&& lambda) {
  static_assert(std::is_invocable_v<LAMBDA, ELEMENTIN>, "the function for fmap is not invocable");
  CONTAINEROUT<std::invoke_result_t<LAMBDA, ELEMENTIN>> res;
  std::transform(args.cbegin(), args.cend(), std::back_inserter(res),
                 std::forward<decltype(lambda)>(lambda));
  return std::move(res);
}

template <template <class...> class CONTAINER, typename RES, typename LAMBDA>
auto foldl(CONTAINER<RES> input, LAMBDA&& lambda) {
  return std::accumulate(input.cbegin(), input.cend(), RES{},
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