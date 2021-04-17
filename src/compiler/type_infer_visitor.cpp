/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/type_infer_visitor.hpp"

namespace mimium {
//返り値を考慮しないvoid関数専用の簡易zipWith
template <typename T, typename F>
void zipWith(List<T>& a, List<T>& b, F&& lambda) {
  assert(a.size() == b.size());
  auto&& iter_a = std::begin(a);
  auto&& iter_b = std::begin(b);
  for (int i = 0; i < a.size(); i++) { lambda(*iter_a, *iter_b); }
}

template <typename T>
using boxenabler = std::enable_if_t<isBoxed(T())>;
// Box型やAliasの扱いなどが辛いがテンプレートを毎回書くのが辛いので、overloaded的にラムダと組み合わせる
template <class... Ts>
struct UnifyVisitor : Ts... {
  using Ts::operator()...;
  template <class T>
  using isIntermediate = std::is_same<T, IType::Intermediate>;
  template <class T>
  using isAlias = std::is_same<T, IType::Alias>;

  template <typename T, class U>
  void operator()(T& a, U& b) {
    if constexpr (is_boxed_t<T>::value && is_boxed_t<T>::value) {
      (*this)(a.getraw(), b.getraw());
    } else if constexpr (is_boxed_t<T>::value) {
      (*this)(a.getraw(), b);
    } else if constexpr (is_boxed_t<U>::value) {
      (*this)(a, b.getraw());
    } else if constexpr (std::is_same_v<T, U>) {
      // do nothing
    } else if constexpr (isAlias<T>::value && isAlias<U>::value) {
      TypeUnifier::unify(a.v.v, b.v.v);
    } else if constexpr (isAlias<T>::value) {
      IType::Value tmpb = IType::Value{b};
      TypeUnifier::unify(a.v.v, tmpb);
      b = std::get<U>(tmpb.v);
    } else if constexpr (isAlias<U>::value) {
      IType::Value tmpa = IType::Value{a};
      TypeUnifier::unify(tmpa, b.v.v);
      a = std::get<T>(tmpa.v);
    } else if constexpr (isIntermediate<T>::value && isIntermediate<U>::value) {
      *a.type_id = *b.type_id;  //ポインタの中身を合わせる
      a.type_id = b.type_id;    //ポインタ自体も揃えてしまう
    } else if constexpr (isIntermediate<T>::value) {
      //なんかメインの処理
    } else if constexpr (isIntermediate<U>::value) {
      (*this)(b, a);
    } else {
      throw std::runtime_error("type check error");
    }
  }
};
template <class... Ts>
UnifyVisitor(Ts...) -> UnifyVisitor<Ts...>;

void TypeUnifier::unify(IType::Value& a, IType::Value& b) {
  auto&& unifylambda = [&](IType::Value& a, IType::Value& b) -> void { unify(a, b); };
  UnifyVisitor vis{
      [&](IType::Tuple& a, IType::Tuple& b) -> void { zipWith(a.v, b.v, unifylambda); },
      [&](IType::Variant& a, IType::Variant& b) -> void { zipWith(a.v, b.v, unifylambda); },
      [&](IType::Record& a, IType::Record& b) -> void {
        auto&& iter_a = std::begin(a.v);
        auto&& iter_b = std::begin(b.v);
        for (int i = 0; i < a.v.size(); i++) {
          if (iter_a->key != iter_b->key) { throw std::runtime_error("type check error"); }
          unify(iter_a->v, iter_b->v);
          std::advance(iter_a, 1);
          std::advance(iter_b, 1);
        }
      },
      [&](IType::Function& a, IType::Function& b) -> void {
        zipWith(a.v.first, b.v.first, unifylambda);
        unify(a.v.second, a.v.second);
      },
      [&](IType::Array& a, IType::Array& b) -> void {
        if (a.v.size != b.v.size) { throw std::runtime_error("array sizes are different"); }
        unify(a.v.v, b.v.v);
      },
      [&](IType::ListT& a, IType::ListT& b) -> void { unify(a.v, b.v); },
      [&](IType::Identified& a, IType::Identified& b) -> void {
        if (a.v.id != b.v.id) { throw std::runtime_error("identified types are different"); }
        unify(a.v.v, b.v.v);
      },

  };

  std::visit(vis, a.v, b.v);
}

}  // namespace mimium
