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

struct TypeResolver {
  // TypeIdの集合と束縛のリスト
  template <class T>
  using Binding = Pair<T, T>;

  using Constraints = Set<Binding<IType>>;
  using map = Map<IType, HType>;
};

struct TypeInferer {
  using InputType = IType;
  template <class T>
  using TypeEnv = Environment<LAst::expr, T>;

  using TypeEnvI = TypeEnv<IType>;
  using InputAliasMap = Map<std::string, IType>;
  // ASTを入れると中間変数を含む型環境とエイリアスの写像が帰ってくる
  using Pass1 = Pair<TypeEnv<IType>, InputAliasMap>(LAst::expr);

  // エイリアスを全部剥がした型環境を返す
  using Pass2 = TypeEnv<IType>(TypeEnv<IType>, InputAliasMap);
  //中間変数を含む型環境を受け取り、変数同士の制約を作る
  using Pass3 = TypeResolver::Constraints(TypeEnv<IType>);
  //変数の制約を受け取り、中間変数を含む型集合->中間変数を取り除いた型集合 の写像をつくる
  using Pass4 = TypeResolver::map(TypeResolver::Constraints);
  //中間変数を含む型環境と、中間変数を含む型集合->中間変数を取り除いた型集合の写像を受け取り、
  //中間変数を取り除いた型環境を返す
  using Pass5 = TypeEnv<HType>(TypeEnv<IType>, TypeResolver::map);

  // 全体としては、ASTを入れると中間変数がない(型|Alias)の環境が帰ってくる
  using type = TypeEnv<HType>(LAst::expr);


  TypeEnv<HType> infer(LAst::expr const& expr);
};


struct TypeUnifier {
  
  static void unify(IType::Value& a, IType::Value& b);
};

}  // namespace mimium