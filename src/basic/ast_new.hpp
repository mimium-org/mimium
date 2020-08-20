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
struct TypeSpec;

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

using Expr = std::variant<Op, Number, String, Rvar, Self, Rec_Wrap<Lambda>,
                          Rec_Wrap<Fcall>, Rec_Wrap<Time>, Rec_Wrap<Struct>,
                          Rec_Wrap<StructAccess>, Rec_Wrap<ArrayInit>,
                          Rec_Wrap<ArrayAccess>, Rec_Wrap<Tuple>>;
using ExprPtr = std::shared_ptr<Expr>;

struct Assign;
struct Return;
struct Declaration;
struct For;
struct If;

using Statement = std::variant<Assign, Return, /* Declaration, */
                               Rec_Wrap<For>, Rec_Wrap<If>, Rec_Wrap<ExprPtr>>;
using Statements = std::deque<std::shared_ptr<Statement>>;

enum class OpId {
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  Exponent,
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
  Not,
  LShift,
  RShift
};

inline const std::map<OpId, std::string_view> op_str = {
    {OpId::Add, "Add"},
    {OpId::Sub, "Sub"},
    {OpId::Mul, "Mul"},
    {OpId::Div, "Div"},
    {OpId::Mod, "Mod"},
    {OpId::Exponent, "Exponent"},
    {OpId::Equal, "Equal"},
    {OpId::NotEq, "NotEq"},
    {OpId::LessEq, "LessEq"},
    {OpId::GreaterEq, "GreaterEq"},
    {OpId::LessThan, "LessThan"},
    {OpId::GreaterThan, "GreaterThan"},
    {OpId::And, "And"},
    {OpId::BitAnd, "BitAnd"},
    {OpId::Or, "Or"},
    {OpId::BitOr, "BitOr"},
    {OpId::Xor, "Xor"},
    {OpId::Not, "Not"},
    {OpId::LShift, "LShift"},
    {OpId::RShift, "RShift"}};

struct Pos {
  int line;
  int col;
};
struct SourceLoc {
  Pos begin;
  Pos end;
};

inline std::ostream& operator<<(std::ostream& os, const SourceLoc& loc) {
  os << loc.begin.line << ":" << loc.begin.col << " ~ " << loc.end.line << ":"
     << loc.end.col;
  return os;
}

struct DebugInfo {
  SourceLoc source_loc;
  std::string symbol;
};

// Base definition of ast. all ast must be derived from this class

struct Base {
  DebugInfo debuginfo;
};

// derived AST classes are designed to be initialized with nested aggregate
// initialization (C++17 feature) like below: Op opast = {{dbginfo}, operator,
// lhs_ptr, rhs_ptr };

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

// Type Specifier Ast. Typename may be omitted or user-defined.

struct TypeSpec : public Symbol {};

struct Lvar : public Symbol {
  std::optional<types::Value> type;
};

struct Rvar : public Symbol {};
struct Self : public Base {
  std::optional<types::Value> type;  // will be captured at typeinference stage-
                                     // and used at mirgen stage.
};

struct LambdaArgs : public Base {
  std::deque<Lvar> args;
};
struct Lambda : public Base {
  LambdaArgs args;
  Statements body;
  std::optional<types::Value> ret_type;
};

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

struct ArrayInit : public Base {
  std::deque<ExprPtr> args;
};

struct ArrayAccess : public Base {
  ExprPtr array;
  ExprPtr index;
};
struct Struct : public Base {
  std::deque<ExprPtr> args;
};

struct StructAccess : public Base {
  ExprPtr stru;
  ExprPtr field;
};

// Time ast, only a function call can be tied with time.
struct Time : public Base {
  Fcall fcall;
  ExprPtr when;
};

struct Assign : public Base {
  Lvar lvar;
  ExprPtr expr;
};

struct Return : public Base {
  ExprPtr value;
};

struct Declaration : public Base {
  ExprPtr value;
};

struct For : public Base {
  Lvar index;
  ExprPtr iterator;
  Statements statements;
};

struct If : public Base {
  ExprPtr cond;
  Statements then_stmts;
  std::optional<Statements> else_stmts;
};

template <typename FROM, typename TO>
std::shared_ptr<TO> makeAst(FROM&& ast) {
  newast::Expr expr = ast;
  return std::make_shared<TO>(expr);
}

template <typename FROM>
auto makeExpr(FROM&& ast) {
  newast::Expr expr = ast;
  return std::make_shared<newast::Expr>(std::move(expr));
}
template <typename FROM>
auto makeStatement(FROM&& ast) {
  newast::Statement stmt = ast;
  return std::make_shared<newast::Statement>(std::move(stmt));
}

}  // namespace newast
}  // namespace mimium