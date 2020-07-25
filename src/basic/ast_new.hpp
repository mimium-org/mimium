/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "basic/helper_functions.hpp"
#include "basic/type.hpp"
using mmmfloat = double;

namespace mimium {
namespace newast {

// forward declaration
struct Base;
struct Op;
struct Number;
struct String;

struct Symbol;
struct Lvar;
struct Rvar;
struct Self;

struct Lambda;
struct Fcall;

struct Struct;  // currently internally used for closure conversion;
struct StructAccess;

struct LambdaArgs;
struct FcallArgs;
struct ArrayInit;
struct Tuple;

struct ArrayAccess;

struct Time;

using Expr =
    std::variant<Op, Number, String, Rvar, Self, Rec_Wrap<Lambda>,
                 Rec_Wrap<Fcall>, Rec_Wrap<Time>, Rec_Wrap<StructAccess>,
                 Rec_Wrap<ArrayInit>, Rec_Wrap<ArrayAccess>, Rec_Wrap<Tuple>>;

struct Fdef;  // internally equivalent to lambda
struct Assign;
struct Return;
struct Declaration;
struct For;
struct If;

using Statement = std::variant<Assign, Return, Declaration, Rec_Wrap<Fdef>,
                               Rec_Wrap<For>, Rec_Wrap<If>, Rec_Wrap<Expr>>;
using Statements = std::deque<Statement>;

using ExprPtr = std::shared_ptr<Expr>;

enum class OpId {
  Add,
  Sub,
  Mul,
  Div,
  Equal,
  NotEq,
  LessEq,
  GreaterEq,
  LessThan,
  GreaterThan,
  And,
  BitAnd,
  Or,
  BitOr,
  Xor,
  Exponent,
  Not,
  LShift,
  RShift
};

struct DebugInfo {
  struct SourceLoc {
    int line;
    int col;
  } source_loc;
  std::string symbol;
};

// Base definition of ast. all ast must be derived from this class

struct Base {
  DebugInfo debuginfo;
};

// derived AST classes are designed to be initialized with nested aggregate initialization (C++17 feature) like below:
// Op opast = {{dbginfo}, lhs_ptr, rhs_ptr };

// Operator ast. lhs might be nullopt in case of Sub and Not operator.

struct Op : public Base { 
  OpId op;
  std::optional<ExprPtr> lhs;
  ExprPtr rhs;
};
struct Number : public Base {
  mmmfloat value{};
};
struct Symbol : public Base {
  std::string value{};
};
struct String : public Symbol {};
struct Lvar : public Symbol {};
struct Rvar : public Symbol {};
struct Self : public Base {};

struct LambdaArgs : public Base {
  std::deque<std::shared_ptr<Lvar>> args;
};
struct Lambda : public Base {
  std::shared_ptr<LambdaArgs> args;
  std::shared_ptr<Statements> body;
};
struct Fdef : public Lambda {};

struct FcallArgs : public Base {
  std::deque<ExprPtr> args;
};

// Fcall ast, callee may be not simply function name but lambda definition or
// high order function

struct Fcall : public Base {
  ExprPtr fn;
  FcallArgs args;
};

struct Tuple : public Base {
  std::deque<ExprPtr> args;
};

struct ArrayInit  : public Base {
  std::deque<ExprPtr> args;
};

struct ArrayAccess : public Base {
  std::shared_ptr<Expr> array;
  std::shared_ptr<Expr> index;
};
struct Struct  : public Base {
  std::deque<ExprPtr> args;
};

struct StructAccess : public Base {
  std::shared_ptr<Expr> array;
  std::shared_ptr<Expr> index;
};

// Time ast, only a function call can be tied with time.
struct Time : public Base {
  std::shared_ptr<Fcall> fcall;
  mmmfloat time;
};

struct Assign : public Base{
    std::shared_ptr<Lvar> lvar;
    ExprPtr expr;
};

template <typename FROM,typename TO>
std::shared_ptr<TO> makeAst(FROM&& ast){
    newast::Expr expr = ast;
    return std::make_shared<TO>(expr);
}

template <typename FROM>
auto makeExpr(FROM&& ast){
    return std::make_shared<newast::Expr>(newast::Expr(ast));
}
template <typename FROM>
auto makeStatement(FROM&& ast){
    return std::make_shared<newast::Statement>(newast::Statement(ast));
}


}  // namespace newast
}  // namespace mimium