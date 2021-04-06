#pragma once
#include <list>
#include <unordered_map>
#include <unordered_set>

#include "basic/helper_functions.hpp"
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

template<class T,class U>
using Pair = std::pair<T,U>;

template <class R, class... ArgTypes>
using Fn = std::function<R(ArgTypes...)>;

template <class T,class U>
using Either = std::variant<T,U>;

template <class T, template <class...> class C, template <class...> class... Cs>
struct Nested {
  using type = C<Nested<T, Cs...>>;
};
}  // namespace mimium