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
};

using types = MakeRecursive<Type>::type;

template <class ID, class Value>
struct Alias {
  ID name;
  Value v;
};

struct TypeResolver {
  using HType = types::HType;

  // TypeIdの集合と束縛のリスト
  template <class T>
  using Binding = std::pair<T, T>;
  using IType = types::IType;
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
  // ASTを入れると中間変数を含む型環境が帰ってくる
  using Pass1 = TypeEnv<types::IType>(AST);
  //中間変数を含む型環境を受け取り、変数同士の制約を作る
  using Pass2 = TypeResolver::Constraints(std::result_of_t<Pass1>);
  //変数の制約を受け取り、中間変数を含む型集合->中間変数を取り除いた型集合 の写像をつくる
  using Pass3 = TypeResolver::map(std::result_of_t<Pass2>);
  //中間変数を含む型環境と、中間変数を含む型集合->中間変数を取り除いた型集合の写像を受け取り、
  //中間変数を取り除いた型環境を返す
  using Pass4 = TypeEnv<types::HType>(std::result_of_t<Pass1>, std::result_of_t<Pass3>);
  //全体としては、ASTを入れると中間変数がない型環境が帰ってくる
  using type = TypeEnv<types::HType>(AST);
};

}  // namespace mimium