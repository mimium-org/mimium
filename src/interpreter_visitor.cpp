#include "interpreter_visitor.hpp"

namespace mimium {

void InterpreterVisitor::init() {
  rootenv = std::make_shared<Environment>("root", nullptr);

  currentenv = rootenv;  // share
  currentenv->setVariable("dacL", 0);
  currentenv->setVariable("dacR", 0);
}
void InterpreterVisitor::clear() {
  rootenv.reset();
  currentenv.reset();
  clearDriver();
  init();
}

void InterpreterVisitor::start() {
  sch->start();
  running_status = true;
}

void InterpreterVisitor::stop() {
  sch->stop();
  running_status = false;
}

mValue InterpreterVisitor::loadSource(const std::string src) {
  driver.parsestring(src);
  return loadAst(driver.getMainAst());
}
mValue InterpreterVisitor::loadSourceFile(const std::string filename) {
  driver.parsefile(filename);
  return loadAst(driver.getMainAst());
}

mValue InterpreterVisitor::loadAst(AST_Ptr _ast) {
  auto ast = std::dynamic_pointer_cast<ListAST>(_ast);
  return ast->accept(*this);
}

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
    currentenv->setVariable(varname, res);  // share
    Logger::debug_log(
        "Variable " + varname + " : " + InterpreterVisitor::to_string(res),
        Logger::DEBUG);
    return res;  // for print
  } else {
    throw std::runtime_error("expression not resolved");
  }
}
mValue InterpreterVisitor::visit(AbstractListAST& ast) { return 0; };
mValue InterpreterVisitor::visit(ArrayAST& ast) { return 0; };
mValue InterpreterVisitor::visit(ArrayAccessAST& ast) { return 0; };
mValue InterpreterVisitor::visit(FcallAST& ast) { return 0; };
mValue InterpreterVisitor::visit(LambdaAST& ast) { return 0; };
mValue InterpreterVisitor::visit(IfAST& ast) { return 0; };
mValue InterpreterVisitor::visit(ReturnAST& ast) { return 0; };
mValue InterpreterVisitor::visit(ForAST& ast) { return 0; };
mValue InterpreterVisitor::visit(DeclarationAST& ast) { return 0; };
mValue InterpreterVisitor::visit(TimeAST& ast) {
  mValue time = ast.getTime()->accept(*this);
  sch->addTask(std::get<double>(time), ast.getExpr());

  return 0;
};

std::string InterpreterVisitor::to_string(mValue v) {
  return std::visit(overloaded{[](double v) { return std::to_string(v); },
                               [](std::vector<double> vec) {
                                 std::stringstream ss;
                                 ss << "[";
                                 int count = vec.size();
                                 for (auto& elem : vec) {
                                   ss << elem;
                                   count--;
                                   if (count > 0) ss << ",";
                                 }
                                 ss << "]";
                                 return ss.str();
                               },
                               [](auto v) { return v->toString(); } },
                    v);
};

bool InterpreterVisitor::assertArgumentsLength(std::vector<AST_Ptr>& args,
                                        int length) {
  int size = args.size();
  if (size == length) {
    return true;
  } else {
    throw std::runtime_error(
        "Argument length is invalid. Expected: " + std::to_string(length) +
        " given: " + std::to_string(size));
    return false;
  };
}
}  // namespace mimium