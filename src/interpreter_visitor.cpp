#include "interpreter_visitor.hpp"

namespace mimium {
mValue InterpreterVisitor::visit(ListAST& ast) {
  mValue res;
  for (auto& line : ast.getlist()) {
    res = line->accept(*this);
  }
  return res;
}
mValue InterpreterVisitor::visit(NumberAST& ast) { return ast.getVal(); };
mValue InterpreterVisitor::visit(SymbolAST& ast) { return 0; };
mValue InterpreterVisitor::visit(OpAST& ast) {
  double lv = std::get<double>(ast.lhs->accept(*this));
  double rv = std::get<double>(ast.rhs->accept(*this));
  switch (ast.getOpId()) {
    case ADD:
      return lv + rv;
      break;
    case SUB:
      return lv - rv;
      break;
    case MUL:
      return lv * rv;
      break;
    case DIV:
      return lv / rv;
      break;
    case EXP:
      return std::pow(lv, rv);
      break;
    case MOD:
      return std::fmod(lv, rv);
      break;
    case AND:
    case BITAND:
      return (double)((bool)lv & (bool)rv);
      break;
    case OR:
    case BITOR:
      return (double)((bool)lv | (bool)rv);
      break;
    case LT:
      return (double)lv < rv;
      break;
    case GT:
      return (double)lv > rv;
      break;
    case LE:
      return (double)lv <= rv;
      break;
    case GE:
      return (double)lv >= rv;
      break;
    case LSHIFT:
      return (double)((int)lv << (int)rv);
      break;
    case RSHIFT:
      return (double)((int)lv >> (int)rv);
      break;
    default:
      throw std::runtime_error("invalid binary operator");
      return 0.0;
  }
}
mValue InterpreterVisitor::visit(AssignAST& ast) {
  std::string varname = ast.getName()->getVal();  // assuming name as symbolast
  auto body = ast.getBody();

  if (body) {
    mValue res = body->accept(*this);
    itp->getCurrentEnv()->setVariable(varname, res);  // share
    Logger::debug_log(
        "Variable " + varname + " : " + Interpreter::to_string(res),
        Logger::DEBUG);
    return res;  // for print
  } else {
    throw std::runtime_error("expression not resolved");
  }
}
mValue InterpreterVisitor::visit(AbstractListAST& ast){return 0;};
mValue InterpreterVisitor::visit(ArrayAST& ast){return 0;};
mValue InterpreterVisitor::visit(ArrayAccessAST& ast){return 0;};
mValue InterpreterVisitor::visit(FcallAST& ast){return 0;};
mValue InterpreterVisitor::visit(LambdaAST& ast){return 0;};
mValue InterpreterVisitor::visit(IfAST& ast){return 0;};
mValue InterpreterVisitor::visit(ReturnAST& ast){return 0;};
mValue InterpreterVisitor::visit(ForAST& ast){return 0;};
mValue InterpreterVisitor::visit(DeclarationAST& ast){return 0;};
mValue InterpreterVisitor::visit(TimeAST& ast){return 0;};
}  // namespace mimium