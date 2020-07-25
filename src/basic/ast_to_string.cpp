/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ast_to_string.hpp"
namespace mimium {

ToStringVisitor::ToStringVisitor(std::ostream& output, Mode mode)
    : output(output) {
  this->mode = mode;
  switch (mode) {
    case Mode::Lisp:
      format = {"(", ")", "(", ")", " "};
      break;
    case Mode::Json:
      format = {"{", "}", "[", "]", " , "};
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
  StatementStringVisitor svisitor(os,Mode::Lisp);
  for (const auto& statement : statements) {
    std::visit(svisitor, *statement);
  }
  os << std::flush;
  return os;
}
void ExprStringVisitor::operator()(newast::Number ast) {
  output << ast.value;
}
void ExprStringVisitor::operator()(newast::String ast) {
  output << ast.value;
}
void ExprStringVisitor::operator()(newast::Op ast) {
  output << format.lpar;
  if(ast.lhs){
    std::visit(*this,*ast.lhs.value());
  }
  output << format.delim;
  std::visit(*this,*ast.rhs);
}
void ExprStringVisitor::operator()(newast::Rvar ast) {
  output << ast.value;
}
void ExprStringVisitor::operator()(newast::Self  /*ast*/) {
  output << "self";
}
void ExprStringVisitor::operator()(Rec_Wrap<newast::Lambda> ast) {
  auto lambda = ast.getraw();
  output << format.lpar <<" lambda" <<format.delim;
  auto largs = *lambda.args;
  output << format.lpar_a<< joinVec(largs.args,format.delim) << format.rpar_a;
  output << *lambda.body;
}

void ExprStringVisitor::operator()(Rec_Wrap<newast::Fcall> ast) {}
void ExprStringVisitor::operator()(Rec_Wrap<newast::Time> ast) {}
void ExprStringVisitor::operator()(Rec_Wrap<newast::StructAccess> ast) {}
void ExprStringVisitor::operator()(Rec_Wrap<newast::ArrayInit> ast) {}
void ExprStringVisitor::operator()(Rec_Wrap<newast::ArrayAccess> ast) {}
void ExprStringVisitor::operator()(Rec_Wrap<newast::Tuple> ast) {}
void StatementStringVisitor::operator()(newast::Assign ast) {}
void StatementStringVisitor::operator()(newast::Return ast) {}
void StatementStringVisitor::operator()(newast::Declaration ast) {}
void StatementStringVisitor::operator()(Rec_Wrap<newast::Fdef> ast) {}
void StatementStringVisitor::operator()(Rec_Wrap<newast::For> ast) {}
void StatementStringVisitor::operator()(Rec_Wrap<newast::If> ast) {}
void StatementStringVisitor::operator()(Rec_Wrap<newast::Expr> ast) {}
}  // namespace mimium