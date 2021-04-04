#pragma once

#include <list>
#include "type.hpp"

namespace mimium {
namespace ast_defs {

template <class T>
using IdType = T;

}
namespace ast {
template<class T>
struct Primitive{
    T v;
};

using FloatLit =Primitive<float>;
using IntLit =Primitive<int>;
using BoolLit = Primitive<bool>;
using StringLit = Primitive<std::string>;

struct Symbol{
    std::string v;
};
template <class EXPR>
struct TupleLit;
template <class EXPR>
struct StructLit;
template <class EXPR>
struct ArrayLit;
template <class EXPR>
struct Lambda;
template <class EXPR>
struct Let;
template <class EXPR>
struct If;
// clang-format off
template <typename Expr,typename... T>
using ExprBase = std::variant<FloatLit, 
                                IntLit, 
                                BoolLit, 
                                StringLit, 
                                Symbol, 
                                TupleLit<Expr>, 
                                StructLit<Expr>,
                                ArrayLit<Expr>, 
                                Lambda<Expr>, 
                                If<Expr>, 
                                T...>;
// clang-format on

struct RecPlaceHolder {};

template <template <class> class Kind>
struct MakeRec {
  using type = Box<Kind<MakeRec>>;
};

template <class EXPR>
struct App;

template <class EXPR>
using ExprPrim = ExprBase<App<EXPR>, Let<EXPR>>;

using Expr = MakeRec<ExprPrim>::type;

template <class EXPR>
struct App {
  EXPR callee;
  std::list<EXPR> args;
};

template <class EXPR>
struct TupleLit{

}; 
template <class EXPR>
struct StructLit{

}; 
template <class EXPR>
struct ArrayLit{

}; 
template <class EXPR>
struct Lambda{

}; 
template <class EXPR>
struct Let{

}; 
template <class EXPR>
struct If{
     EXPR e;
}; 


Expr e{FloatLit{0}};
auto& test = std::get<FloatLit>(*e.t);

// HIGH-LEVEL AST including Syntactic Sugar

struct AppCurry;

struct Infix;
struct Schedule;  //@ operator
struct EnvVar;    // #hoge
struct Block;

using ExprWithSugar = ExprBase<Box<Infix>, Box<Schedule>, Box<EnvVar>, Box<AppCurry>, Box<Block>>;

// STATEMENTS

struct Assignment;
struct DefFn;
struct ExtFun;

using Statement = std::variant<Box<Assignment>, Box<DefFn>, Box<ExtFun>>;
using Statements = std::list<Statement>;

struct TypeAlias;  // type name = hoge

}  // namespace ast

}  // namespace mimium