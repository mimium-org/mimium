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

std::ostream& operator<<(std::ostream& os,
                         const newast::Statements& statements) {
  StatementStringVisitor svisitor(os, Mode::Lisp);
  for (const auto& statement : statements) {
    std::visit(svisitor, *statement);
  }
  os << std::flush;
  return os;
}
void ExprStringVisitor::operator()(const newast::Number& ast) {
  output << ast.value;
}
void ExprStringVisitor::operator()(const newast::String& ast) {
  output << ast.value;
}
void ExprStringVisitor::operator()(const newast::Op& ast) {
  output << format.lpar;
  if (ast.lhs) {
    std::visit(*this, *ast.lhs.value());
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
  output << format.lpar << "funcall" << format.delim;
  auto fargs = fcall.args;
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
void ExprStringVisitor::operator()(const Rec_Wrap<newast::StructAccess>& ast) {
  const newast::StructAccess& stracc = ast;
  output << format.lpar << "structaccess" << format.delim;
  std::visit(*this, *stracc.stru);
  output << format.delim;
  std::visit(*this, *stracc.field);
  output << format.rpar;
}
void ExprStringVisitor::operator()(const Rec_Wrap<newast::ArrayInit>& ast) {}
void ExprStringVisitor::operator()(const Rec_Wrap<newast::ArrayAccess>& ast) {}
void ExprStringVisitor::operator()(const Rec_Wrap<newast::Tuple>& ast) {}
void StatementStringVisitor::operator()(const newast::Assign& ast) {}
void StatementStringVisitor::operator()(const newast::Return& ast) {}
void StatementStringVisitor::operator()(const newast::Declaration& ast) {}
void StatementStringVisitor::operator()(const Rec_Wrap<newast::Fdef>& ast) {}
void StatementStringVisitor::operator()(const Rec_Wrap<newast::For>& ast) {}
void StatementStringVisitor::operator()(const Rec_Wrap<newast::If>& ast) {}
void StatementStringVisitor::operator()(const Rec_Wrap<newast::ExprPtr>& ast) {}


}  // namespace mimium