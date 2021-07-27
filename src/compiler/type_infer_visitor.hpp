/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/ast_new.hpp"
#include "basic/environment.hpp"
#include "basic/type_new.hpp"

#include "compiler/ffi.hpp"
// type inference ... assumed to be visited after finished alpha-conversion(each
// variable has unique name regardless its scope)

namespace mimium {
using IntPtr = std::shared_ptr<int>;
template <class T>
using TypeEnv = Environment<decltype(LAst::Id::v), std::shared_ptr<T>>;

using TypeEnvI = TypeEnv<IType::Value>;
using TypeEnvH = TypeEnv<HType::Value>;
struct TypeResolver {
  // TypeIdの集合と束縛のリスト
  template <class T>
  using Binding = Pair<T, T>;

  using Constraints = Set<Binding<IType>>;
  using map = Map<IType, HType>;
};
template <typename... Ts>
struct UnifyVisitor;
struct TypeInferer {
  template <typename...>
  friend struct UnifyVisitor;
  using InputType = IType;

  using InputAliasMap = Map<std::string, IType>;
  // ASTを入れると中間変数を含む型環境とエイリアスの写像が帰ってくる
  using Pass1 = Pair<TypeEnvI, InputAliasMap>(LAst::expr);

  // エイリアスを全部剥がした型環境を返す
  using Pass2 = TypeEnvI(TypeEnvI, InputAliasMap);
  //中間変数を含む型環境を受け取り、変数同士の制約を作る
  using Pass3 = TypeResolver::Constraints(TypeEnvI);
  //変数の制約を受け取り、中間変数を含む型集合->中間変数を取り除いた型集合 の写像をつくる
  using Pass4 = TypeResolver::map(TypeResolver::Constraints);
  //中間変数を含む型環境と、中間変数を含む型集合->中間変数を取り除いた型集合の写像を受け取り、
  //中間変数を取り除いた型環境を返す
  using Pass5 = TypeEnv<HType>(TypeEnvI, TypeResolver::map);

  // 全体としては、ASTを入れると中間変数がない(型|Alias)の環境が帰ってくる
  using type = TypeEnv<HType>(LAst::expr);


  using ITypePtr = std::shared_ptr<IType::Value>;
  TypeEnvH infer(LAst::expr& expr);
  ITypePtr inferInternal(LAst::expr& expr, std::shared_ptr<TypeEnvI> env, int level);
  void unify(IType::Value& a, IType::Value& b);
  static bool occurCheck(IType::Intermediate const& lv, IType::Value& rv);

 private:
  ITypePtr self_t_holder = nullptr;

  ITypePtr generalize(ITypePtr const& t, int level);
  ITypePtr generalizeInternal(ITypePtr const& t, int level,
                                  Map<int, IntPtr>& typevar_to_scheme);

  ITypePtr instantiate(ITypePtr const& t, int level);
  ITypePtr instantiateInternal(ITypePtr const& t, int level,
                                   Map<int, IntPtr>& scheme_to_typevar);
  HType::Value lowerIType(IType::Value const& v, Map<IntPtr, IType::Value> const& typevar_map);
  std::shared_ptr<TypeEnv<HType::Value>> substituteIntermediateVars(
      std::shared_ptr<TypeEnvI> env, Map<IntPtr, IType::Value> const& typevar_map);

  int typevar_count = 0;
  int typescheme_count = 0;
  Map<int, IntPtr> scheme_to_typevar;
  Map<int, IntPtr> typevar_to_scheme;

  Map<IntPtr, IType::Value> typevar_to_val;
  IType::Intermediate makeNewTypeVar(int level);
  IType::TypeScheme makeNewTypeScheme();
};

}  // namespace mimium