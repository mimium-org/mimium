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

namespace newast{
std::ostream& operator<<(std::ostream& os, const newast::Expr& expr) {
  std::visit(ExprStringVisitor(os, Mode::Lisp), expr);
  return os;
}
std::ostream& toString(std::ostream& os, const newast::Statement& statement) {
  StatementStringVisitor svisitor(os, Mode::Lisp);
  std::visit(svisitor,statement);
  return os;
}

std::ostream& operator<<(std::ostream& os, const newast::Lvar& lvar){
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

std::ostream& operator<<(std::ostream& os,
                                const newast::Statements& statements){
  StatementStringVisitor svisitor(os, Mode::Lisp);
  for (const auto& statement : statements) {
    std::visit(svisitor, *statement);
  }
  os << std::flush;
  return os;
}
}  // namespace newast

void ExprStringVisitor::operator()(const newast::Number& ast) {
  output << ast.value;
}
void ExprStringVisitor::operator()(const newast::String& ast) {
  output << ast.value;
}
void ExprStringVisitor::operator()(const newast::Op& ast) {
  output << format.lpar;
  if (ast.lhs.has_value()) {
    output << ast.lhs.value();
  }
  output << format.delim;
  std::visit(*this, *ast.rhs);
}
void ExprStringVisitor::operator()(const newast::Rvar& ast) {
  output << ast.value;
}
void ExprStringVisitor::operator()(const newast::Self& /*ast*/) {
  output << "self";
}
void ExprStringVisitor::operator()(const Rec_Wrap<newast::Lambda>& ast) {
  const newast::Lambda& lambda = ast;
  output << format.lpar << "lambda" << format.delim;
  auto largs = lambda.args;
  output << format.lpar_a << joinVec(largs.args, format.delim) << format.rpar_a
         << format.br;
  output << lambda.body << format.rpar_a;
}
void ExprStringVisitor::fcallHelper(const newast::Fcall& fcall) {
  output << format.lpar_a << "funcall" << format.delim;
  auto&& fargs = fcall.args;
  output << joinVec(fargs.args, format.delim) << format.rpar_a;
}

void ExprStringVisitor::operator()(const Rec_Wrap<newast::Fcall>& ast) {
  const newast::Fcall& fcall = ast;
  fcallHelper(fcall);
}
void ExprStringVisitor::operator()(const Rec_Wrap<newast::Time>& ast) {
  const newast::Time& time = ast;
  output << format.lpar << "time" << format.delim;
  fcallHelper(time.fcall);
  output << format.delim << format.rpar;
}
void ExprStringVisitor::operator()(const Rec_Wrap<newast::Struct>& ast) {
  const newast::Struct& str = ast;
  output << format.lpar << "struct" << format.delim;
  output << joinVec(str.args, format.delim) << format.rpar;
}
void ExprStringVisitor::operator()(const Rec_Wrap<newast::StructAccess>& ast) {
  const newast::StructAccess& stracc = ast;
  output << format.lpar << "structaccess" << format.delim << format.lpar_a
         << *stracc.stru << format.delim << *stracc.field << format.rpar_a
         << format.rpar;
}
void ExprStringVisitor::operator()(const Rec_Wrap<newast::ArrayInit>& ast) {
  const newast::ArrayInit& arr = ast;
  output << format.lpar << "array" << format.delim << format.lpar_a;
  output << joinVec(arr.args, format.delim);
  output << format.rpar_a << format.delim << format.rpar;
}
void ExprStringVisitor::operator()(const Rec_Wrap<newast::ArrayAccess>& ast) {
  const newast::ArrayAccess& acc = ast;
  output << format.lpar << "arrayaccess" << format.delim << *acc.array
         << format.delim << acc.index << format.rpar;
}
void ExprStringVisitor::operator()(const Rec_Wrap<newast::Tuple>& ast) {
  const newast::Tuple& tup = ast;
  output << format.lpar << "tuple" << format.delim << format.lpar_a;
  output << joinVec(tup.args, format.delim);
  output << format.rpar_a << format.delim << format.rpar;
}
void StatementStringVisitor::operator()(const newast::Assign& ast) {
  output << format.lpar << "assign" << format.delim << ast.lvar << format.delim
         << *ast.expr << format.rpar;
}
void StatementStringVisitor::operator()(const newast::Return& ast) {
  output << format.lpar << "return" << format.delim << *ast.value << format.rpar;
}
// void StatementStringVisitor::operator()(const newast::Declaration& ast) {}
void StatementStringVisitor::operator()(const Rec_Wrap<newast::For>& ast) {}
void StatementStringVisitor::operator()(const Rec_Wrap<newast::If>& ast) {
  const newast::If& ifast = ast;
  output << format.lpar << "if" << format.delim << *ifast.cond << format.delim
         << ifast.then_stmts;
  if (ifast.else_stmts.has_value()) {
    output << format.delim << ifast.else_stmts.value();
  }
  output << format.rpar;
}
void StatementStringVisitor::operator()(const Rec_Wrap<newast::ExprPtr>& ast) {
  const newast::ExprPtr& expr = ast;
  output << *expr;
}

}  // namespace mimium