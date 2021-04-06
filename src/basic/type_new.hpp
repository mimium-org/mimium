#pragma once
#include "abstractions.hpp"

namespace mimium {

template <class Value>
struct Type {
  struct Unit {};
  struct Bool {};
  struct Int {};
  struct Float {};
  struct String {};
  template <template <class...> class C, int ID = 0>
  using Aggregate = CategoryWrapped<C, Value, ID>;
  // T1 * T2 * T3 ...
  using Tuple = Aggregate<List>;
  // T1 | T2 | T3...
  using Variant = Aggregate<List, 1>;
  // T1 -> T2 -> T3...
  using Function = Aggregate<List, 2>;

  template <class Identifier>
  struct RecordCategory {
    Identifier key;
    Value v;
  };
  using Record = CategoryWrapped<List, RecordCategory<std::string>>;

  // Identified type
  template <class T>
  struct IdentifiedCategory {
    T id;
    Value v;
  };
  using Identified = typename CategoryWrapped<IdentifiedCategory, int>::type;

  template <class... T>
  using TypeBase = std::variant<Unit, Bool, Int, Float, String, Tuple, Variant, Function, Record,
                                Identified, T...>;

  using Intermediate = typename CategoryWrapped<IdentifiedCategory, int, 1>::type;
  using HType = TypeBase<>;

  using IType = TypeBase<Intermediate>;

  struct LType {};

  // Generic Types
  template <class Id>
  using TypeScheme = typename CategoryWrapped<IdentifiedCategory, Id, 2>::type;
};

using types = MakeRecursive<Type>::type;

template <class ID, class Value>
struct Alias {
  using id = ID;
  using map = Map<ID, Value>;
};

struct TypeResolver {
  using HType = types::HType;

  // TypeIdの集合と束縛のリスト
  template <class T>
  using Binding = std::pair<T, T>;
  using IType = types::IType;
  using ITypeOrAlias = std::variant<Alias<std::string, IType>::id, IType>;
  using Constraints = Set<Binding<IType>>;
  using map = Map<IType, HType>;
};

// 変数と型の写像がスコープごとに
template <class Env>
struct Environment {
  template <class T, class Identifier>
  struct Value {
    Map<Identifier, T> map;
    std::optional<Env> e;
  };
};
template <class T>
using TypeEnv = MakeRecursive<Environment>::type::Value<T, std::string>;

template <class AST>
struct TypeInferer {
  using InputType = Either<TypeResolver::IType, Alias<std::string, TypeResolver::IType>>;
  using InputAliasMap = Alias<std::string, TypeResolver::IType>::map;
  // ASTを入れると中間変数を含む型環境とエイリアスの写像が帰ってくる
  using Pass1 = Pair<TypeEnv<InputType>, InputAliasMap>(AST);

  // エイリアスを全部剥がした型環境を返す
  using Pass2 = TypeEnv<TypeResolver::IType>(TypeEnv<InputType>, InputAliasMap);
  //中間変数を含む型環境を受け取り、変数同士の制約を作る
  using Pass3 = TypeResolver::Constraints(TypeEnv<TypeResolver::IType>);
  //変数の制約を受け取り、中間変数を含む型集合->中間変数を取り除いた型集合 の写像をつくる
  using Pass4 = TypeResolver::map(TypeResolver::Constraints);
  //中間変数を含む型環境と、中間変数を含む型集合->中間変数を取り除いた型集合の写像を受け取り、
  //中間変数を取り除いた型環境を返す
  using Pass5 = TypeEnv<TypeResolver::HType>(TypeEnv<InputType>, TypeResolver::map);

//入力の alias|Itype -> alias|Htype にマップし直す
  using OutputType = Either<TypeResolver::HType, Alias<std::string, TypeResolver::HType>>;
  using OutputAliasMap = Alias<std::string, TypeResolver::HType>::map;
  using Pass6 = Pair<TypeEnv<OutputType>, OutputAliasMap>(TypeEnv<TypeResolver::HType>,
                                                          InputAliasMap);
  //全体としては、ASTを入れると中間変数がない(型|Alias)の環境が帰ってくる
  using type =  Pair<TypeEnv<OutputType>, OutputAliasMap>(AST);
};
struct Test {};
TypeInferer<Test> inferer;

}  // namespace mimium