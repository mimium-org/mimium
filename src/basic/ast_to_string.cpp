/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ast_to_string.hpp"
namespace mimium {

ToStringVisitor::ToStringVisitor(std::ostream& output, Mode mode)
    : output(output), is_prettry(true) {
  this->mode = mode;
  switch (mode) {
    case Mode::Lisp: format = {"(", ")", "(", ")", " ", "\n"}; break;
    case Mode::Json: format = {"{", "}", "[", "]", ", ", "\n"}; break;
    default: break;
  }
}

void AstStringifier::operator()(const ast::Number& ast) { output << ast.value; }
void AstStringifier::operator()(const ast::String& ast) { output << ast.value; }
void AstStringifier::operator()(const ast::Op& ast) {
  output << format.lpar << ast::op_str.at(ast.op);
  if (ast.lhs.has_value()) {
    output << format.delim;
    toString(ast.lhs.value());
  }
  output << format.delim << *ast.rhs << format.rpar;
}
void AstStringifier::operator()(const ast::Rvar& ast) { output << ast.value; }
void AstStringifier::operator()(const ast::Self& /*ast*/) { output << "self"; }
void AstStringifier::operator()(const ast::Lambda& ast) {
  output << format.lpar << "lambda" << format.delim << format.lpar_a;
  const auto& largs = ast.args;
  toStringVec(largs.args);
  output << format.rpar_a;
  (*this)(ast.body);
  output << format.rpar;
}
void AstStringifier::operator()(const ast::Fcall& ast) {
  output << format.lpar << "funcall" << format.delim << *ast.fn << format.delim;
  const auto& fargs = ast.args;
  output << format.lpar_a;
  toStringVec(fargs.args);
  output << format.rpar_a << format.rpar;
}

void AstStringifier::operator()(const ast::Time& ast) {
  output << format.lpar << "time" << format.delim;
  (*this)(ast.fcall);
  output << format.delim << format.rpar;
}
void AstStringifier::operator()(const ast::Struct& ast) {
  output << format.lpar << "struct" << format.delim;
  toStringVec(ast.args);
  output << format.rpar;
}
void AstStringifier::operator()(const ast::StructAccess& ast) {
  output << format.lpar << "structaccess" << format.delim << format.lpar_a;
  toString(ast.stru);
  output << format.delim;
  toString(ast.field);
  output << format.rpar_a << format.rpar;
}
void AstStringifier::operator()(const ast::ArrayInit& ast) {
  output << format.lpar << "array" << format.delim << format.lpar_a;
  toStringVec(ast.args);
  output << format.rpar_a << format.delim << format.rpar;
}
void AstStringifier::operator()(const ast::ArrayAccess& ast) {
  output << format.lpar << "arrayaccess" << format.delim;
  toString(ast.array);
  output << format.delim;
  toString(ast.index);
  output << format.rpar;
}
void AstStringifier::operator()(const ast::Tuple& ast) {
  output << format.lpar << "tuple" << format.delim << format.lpar_a;
  toStringVec(ast.args);
  output << format.rpar_a << format.delim << format.rpar;
}
void AstStringifier::operator()(const ast::Assign& ast) {
  output << format.lpar << "assign" << format.delim;
  toString(ast.lvar);
  output << format.delim;
  toString(ast.expr);
  output << format.rpar;
}
void AstStringifier::operator()(const ast::Fdef& ast) {
  output << format.lpar << "Fdef" << format.delim;
  toString(ast.lvar);
  output << format.delim;
  (*this)(ast.fun);
  output << format.rpar;
}
void AstStringifier::operator()(const ast::If& ast) {
  output << format.lpar << "if" << format.delim;
  toString(ast.cond);
  output << format.delim;
  toString(ast.then_stmts);
  if (ast.else_stmts.has_value()) {
    output << format.delim;
    toString(ast.else_stmts.value());
  }
  output << format.rpar;
}
void AstStringifier::operator()(const ast::Return& ast) {
  output << format.lpar << "return" << format.delim;
  toString(ast.value);
  output << format.rpar;
}
// void AstStringifier::operator()(const ast::Declaration& ast) {}
void AstStringifier::operator()(const ast::For& ast) {}

void AstStringifier::operator()(const ast::Block& ast) {
  toString(ast.stmts);
  if (ast.expr.has_value()) { std::visit(*this, *ast.expr.value()); }
}

// Lvar Related
void AstStringifier::operator()(const ast::ArrayLvar& ast) {
  output << format.lpar << "arrayassign" << format.delim;
  toString(ast.array);
  output << format.delim;
  toString(ast.index);
  output << format.rpar;
}
void AstStringifier::operator()(const ast::TupleLvar& ast) {
  output << format.lpar << "tupleassign" << format.delim << format.lpar_a;
  toStringVec(ast.args);
  output << format.rpar_a << format.delim << format.rpar;
}
void AstStringifier::operator()(const ast::DeclVar& ast) {
  output << format.lpar << "lvar" << format.delim << ast.value;
  if (ast.type.has_value()) {
    output << format.delim << types::toString(ast.type.value());
  } else {
    output << format.delim << "unspecified";
  }
  output << format.rpar;
}

void AstStringifier::toString(const ast::Lvar& ast) { std::visit(*this, ast); }

void AstStringifier::toString(const ast::Expr& ast) { std::visit(*this, ast); }
void AstStringifier::toString(ast::ExprPtr ast) { std::visit(*this, *ast); }

void AstStringifier::toString(const ast::Statement& ast) {
  std::visit(*this, ast);
}
void AstStringifier::toString(const ast::Statements& ast) {
  for (auto&& stmt : ast) {
    toString(*stmt);
    output << format.br;
  }
}

}  // namespace mimium