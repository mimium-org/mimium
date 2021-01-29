/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "helper_functions.hpp"
#include "type.hpp"
using mmmfloat = double;

namespace mimium::ast {

// forward declaration
struct Base;
struct Op;
struct Number;
struct String;

struct Symbol;
struct DeclVar;
struct ArrayLvar;
struct TupleLvar;
using Lvar = std::variant<DeclVar, ArrayLvar, TupleLvar>;
struct TypeSpec;

// struct Rvar;
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

struct If;
struct Block;
using Expr =
    std::variant<Op, Number, String, Symbol, Self, Box<Lambda>, Box<Fcall>, Box<If>, Box<Struct>,
                 Box<StructAccess>, Box<ArrayInit>, Box<ArrayAccess>, Box<Tuple>, Box<Block>>;
using ExprPtr = std::shared_ptr<Expr>;

struct Assign;
struct Fdef;

struct Return;
struct Declaration;
struct For;

using Statement = std::variant<Assign, Return, Fdef, Time, Fcall, Box<If>, /* Declaration, */
                               Box<For>>;
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

std::string_view getOpString(OpId id);
struct Pos {
  int line = 1;
  int col;
};
struct SourceLoc {
  Pos begin;
  Pos end;
};

inline std::ostream& operator<<(std::ostream& os, const SourceLoc& loc) {
  os << loc.begin.line << ":" << loc.begin.col << " ~ " << loc.end.line << ":" << loc.end.col;
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

// Lvar related definitions
struct DeclVar : public Base {
  Symbol value;
  std::optional<types::Value> type;
};

struct ArrayLvar : public Base {
  ExprPtr array;
  ExprPtr index;
};
struct TupleLvar : public Base {
  std::deque<DeclVar> args;
};

struct Self : public Base {
  std::optional<types::Value> type;  // will be captured at typeinference stage-
                                     // and used at mirgen stage.
};
struct Block : public Base {
  Statements stmts;
  std::optional<ExprPtr> expr;
};
struct LambdaArgs : public Base {
  std::deque<DeclVar> args;
};

struct Lambda : public Base {
  LambdaArgs args;
  Block body;
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

struct Fdef : public Base {
  DeclVar lvar;
  Lambda fun;
};

struct Return : public Base {
  ExprPtr value;
};

struct Declaration : public Base {
  ExprPtr value;
};

struct For : public Base {
  DeclVar index;
  ExprPtr iterator;
  Block statements;
};

struct If : public Base {
  ExprPtr cond;
  ExprPtr then_stmts;
  std::optional<ExprPtr> else_stmts;
};
template <typename FROM, typename TO>
std::shared_ptr<TO> makeAst(FROM&& ast) {
  ast::Expr expr = ast;
  return std::make_shared<TO>(expr);
}

template <typename FROM>
auto makeExpr(FROM&& ast) {
  ast::Expr expr = std::move(ast);
  return std::make_shared<ast::Expr>(std::move(expr));
}
template <typename FROM>
auto makeStatement(FROM&& ast) {
  return std::make_shared<ast::Statement>(ast);
}

}  // namespace mimium