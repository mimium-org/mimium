/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ast_to_string.hpp"
namespace mimium {

ToStringVisitor::ToStringVisitor(std::ostream& output, Mode mode)
    : output(output), is_prettry(true) {
  this->mode = mode;
  switch (mode) {
    case Mode::Lisp:
      format = {"(", ")", "(", ")", " ", "\n"};
      break;
    case Mode::Json:
      format = {"{", "}", "[", "]", ", ", "\n"};
      break;
    default:
      break;
  }
}

ExprStringVisitor::ExprStringVisitor(std::ostream& output, Mode mode)
    : ToStringVisitor(output, mode) {}
StatementStringVisitor::StatementStringVisitor(std::ostream& output, Mode mode)
    : ToStringVisitor(output, mode), exprstringvisitor(output, mode) {}

namespace ast {

std::ostream& operator<<(std::ostream& os, const ast::Expr& expr) {
  std::visit(ExprStringVisitor(os, Mode::Lisp), expr);
  return os;
}
std::ostream& toString(std::ostream& os, const ast::Statement& statement) {
  StatementStringVisitor svisitor(os, Mode::Lisp);
  std::visit(svisitor, statement);
  return os;
}

// std::ostream& toString(std::ostream& os,
//                                   ast::Statements const& statements) {
//   StatementStringVisitor svisitor(os, Mode::Lisp);
//   for (const auto& statement : statements) {
//     std::visit(svisitor, *statement);
//     os << svisitor.format.br;
//   }
//   os << std::flush;
//   return os;
// }

std::ostream& operator<<(std::ostream& os, const ast::Lvar& lvar) {
  ExprStringVisitor evisitor(os);
  auto& format = evisitor.format;
  os << format.lpar << "lvar" << format.delim << lvar.value << format.delim;
  if (lvar.type.has_value()) {
    os << types::toString(lvar.type.value());
  } else {
    os << "unspecified";
  }
  os << format.rpar;
  return os;
}

}  // namespace ast

void ExprStringVisitor::operator()(const ast::Number& ast) {
  output << ast.value;
}
void ExprStringVisitor::operator()(const ast::String& ast) {
  output << ast.value;
}
void ExprStringVisitor::operator()(const ast::Op& ast) {
  output << format.lpar << ast::op_str.at(ast.op);
  if (ast.lhs.has_value()) {
    output << format.delim << *ast.lhs.value();
  }
  output << format.delim << *ast.rhs << format.rpar;
}
void ExprStringVisitor::operator()(const ast::Rvar& ast) {
  output << ast.value;
}
void ExprStringVisitor::operator()(const ast::Self& /*ast*/) {
  output << "self";
}
void ExprStringVisitor::operator()(const ast::Lambda& ast) {
  output << format.lpar << "lambda" << format.delim;
  auto& largs = ast.args;
  output << format.lpar_a << joinVec(largs.args, format.delim) << format.rpar_a;
  output << ast.body << format.rpar;
  ;
}
void ExprStringVisitor::fcallHelper(const ast::Fcall& fcall) {
  output << format.lpar << "funcall" << format.delim << *fcall.fn
         << format.delim;
  auto& fargs = fcall.args;
  output << format.lpar_a << joinVec(fargs.args, format.delim) << format.rpar_a
         << format.rpar;
}

void ExprStringVisitor::operator()(const ast::Fcall& ast) {
  fcallHelper(ast);
}
void ExprStringVisitor::operator()(const ast::Time& ast) {
  output << format.lpar << "time" << format.delim;
  fcallHelper(ast.fcall);
  output << format.delim << format.rpar;
}
void ExprStringVisitor::operator()(const ast::Struct& ast) {
  output << format.lpar << "struct" << format.delim;
  output << joinVec(ast.args, format.delim) << format.rpar;
}
void ExprStringVisitor::operator()(const ast::StructAccess& ast) {
  output << format.lpar << "structaccess" << format.delim << format.lpar_a
         << *ast.stru << format.delim << *ast.field << format.rpar_a
         << format.rpar;
}
void ExprStringVisitor::operator()(const ast::ArrayInit& ast) {
  output << format.lpar << "array" << format.delim << format.lpar_a;
  output << joinVec(ast.args, format.delim);
  output << format.rpar_a << format.delim << format.rpar;
}
void ExprStringVisitor::operator()(const ast::ArrayAccess& ast) {
  output << format.lpar << "arrayaccess" << format.delim << *ast.array
         << format.delim << ast.index << format.rpar;
}
void ExprStringVisitor::operator()(const ast::Tuple& ast) {
  output << format.lpar << "tuple" << format.delim << format.lpar_a;
  output << joinVec(ast.args, format.delim);
  output << format.rpar_a << format.delim << format.rpar;
}
void StatementStringVisitor::operator()(const ast::Assign& ast) {
  output << format.lpar << "assign" << format.delim << ast.lvar << format.delim
         << *ast.expr << format.rpar;
}
void StatementStringVisitor::operator()(const ast::Return& ast) {
  output << format.lpar << "return" << format.delim << *ast.value
         << format.rpar;
}
// void StatementStringVisitor::operator()(const ast::Declaration& ast) {}
void StatementStringVisitor::operator()(const ast::For& ast) {}
void StatementStringVisitor::operator()(const ast::If& ast) {
  output << format.lpar << "if" << format.delim << *ast.cond << format.delim
         << ast.then_stmts;
  if (ast.else_stmts.has_value()) {
    output << format.delim << ast.else_stmts.value();
  }
  output << format.rpar;
}
void StatementStringVisitor::operator()(const ast::ExprPtr& ast) {
  output << *ast;
}

}  // namespace mimium