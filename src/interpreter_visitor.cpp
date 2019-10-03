#include "interpreter_visitor.hpp"

namespace mimium {
double InterpreterVisitor::get_as_double(mValue v) {
  return std::visit(
      overloaded{[](double d) { return d; },
                 [this](std::string s) {
                   return get_as_double(this->findVariable(s));
                 },  // recursive!!
                 [](auto a) {
                   throw std::runtime_error("refered variable is not number");
                   return 0.;
                 }},
      v);
}
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
mValue InterpreterVisitor::visit(SymbolAST& ast) { return ast.getVal(); };
mValue InterpreterVisitor::visit(OpAST& ast) {
  double lv = get_as_double(ast.lhs->accept(*this));
  double rv = get_as_double(ast.rhs->accept(*this));
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

mValue InterpreterVisitor::visit(ArgumentsAST& ast) {
  auto ptr = std::make_shared<ArgumentsAST>(ast);
  return std::move(ptr);  // this is stupid
}
mValue InterpreterVisitor::visit(ArrayAST& ast) {
  std::vector<double> v;
  for (auto& elem : ast.getElements()) {
    v.push_back(std::get<double>(elem->accept(*this)));
  }
  return std::move(v);
};
mValue InterpreterVisitor::visit(ArrayAccessAST& ast) {
  auto array =
      findVariable(std::get<std::string>(ast.getName()->accept(*this)));
  auto index = (int)std::get<double>(ast.getIndex()->accept(*this));
  return std::visit(
      overloaded{[&index](std::vector<double> a) -> double { return a[index]; },
                 [](auto e) -> double {
                   throw std::runtime_error(
                       "accessed variable is not an array");
                   return 0;
                 }},
      array);
};
overloaded fcall_visitor{
    [](auto v) -> mClosure_ptr {
      throw std::runtime_error("reffered variable is not a function");
      return nullptr;
    },
    [](std::shared_ptr<Closure> v) -> mClosure_ptr { return v; }};
mValue InterpreterVisitor::visit(FcallAST& ast) {
  auto name = std::get<std::string>(ast.getFname()->accept(*this));
  auto args = ast.getArgs();
  if (Builtin::isBuiltin(name)) {
    auto fn = Builtin::builtin_fntable.at(name);
    return fn(args, this);
  } else {
    auto argsv = args->getElements();
    mClosure_ptr closure = std::visit(fcall_visitor, findVariable(name));
    auto lambda = closure->fun;
    auto originalenv = currentenv;
    auto lambdaargs = std::dynamic_pointer_cast<ArgumentsAST>(
                          std::get<AST_Ptr>(lambda.getArgs()->accept(*this)))
                          ->getElements();
    auto tmpenv =
        closure->env->createNewChild("arg" + name);  // create arguments
    int argscond = lambdaargs.size() - argsv.size();
    if (argscond < 0) {
      throw std::runtime_error("too many arguments");
    } else {
      int count = 0;
      for (auto& larg : lambdaargs) {
        auto key = std::get<std::string>(larg->accept(*this));
        tmpenv->setVariableRaw(key, argsv[count]->accept(*this));
        count++;
      }
      if (argscond == 0) {
        currentenv = tmpenv;  // switch env
        auto res = lambda.getBody()->accept(*this);
        currentenv->getParent()->deleteLastChild();
        currentenv = originalenv;
        return res;
      } else {
        throw std::runtime_error(
            "too few arguments");  // ideally we want to return new function
                                   // (partial application)
      }
    }
  }
};
mValue InterpreterVisitor::visit(LambdaAST& ast) {
  auto closure = std::make_shared<Closure>(currentenv, ast);
  return std::move(closure);
};
mValue InterpreterVisitor::visit(IfAST& ast) {
  mValue cond = ast.getCond()->accept(*this);
  auto cond_d = get_as_double(cond);
  if (cond_d > 0) {
    return ast.getThen()->accept(*this);
  } else {
    return ast.getElse()->accept(*this);
  }
};
mValue InterpreterVisitor::visit(ReturnAST& ast) {
  return ast.getExpr()->accept(*this);
};
void InterpreterVisitor::doForVisitor(mValue v, std::string iname,
                                      AST_Ptr expression) {
  std::visit(
      overloaded{
          [&](std::vector<double> v) {
            auto it = v.begin();
            while (it != v.end()) {
              currentenv->setVariable(iname, *it);
              expression->accept(*this);
              it++;
            }
          },
          [&](double v) {
            currentenv->setVariable(iname, v);
            expression->accept(*this);
          },
          [&](std::string s) { doForVisitor(findVariable(s),iname,expression); },
          [](auto v) { throw std::runtime_error("iterator is invalid"); }},
      v);
}

mValue InterpreterVisitor::visit(ForAST& ast) {
  std::string loopname = "for" + std::to_string(rand());  // temporary,,
  currentenv = currentenv->createNewChild(loopname);
  auto varname = std::get<std::string>(ast.getVar()->accept(*this));
  auto expression = ast.getExpression();
  mValue iterator = ast.getIterator()->accept(*this);
  doForVisitor(iterator,varname,expression);
  currentenv = currentenv->getParent();
  return 0.0;  // forloop does not return value
};
mValue InterpreterVisitor::visit(DeclarationAST& ast) { 
  std::string name = std::get<std::string> (ast.getFname()->accept(*this));
  auto argsast = ast.getArgs();
  auto args = argsast->getElements();
  if (name == "include") {
    assertArgumentsLength(args, 1);
    if (args[0]->getid() == SYMBOL) {//this is not smart
      std::string filename = std::get<std::string>(args[0]->accept(*this));
      auto temporary_driver =
          std::make_unique<mmmpsr::MimiumDriver>(current_working_directory);
      temporary_driver->parsefile(filename);
      loadAst(temporary_driver->getMainAst());
      return 0;
    } else {
      throw std::runtime_error("given argument is not a string");
      return 1;
    }
  } else {
    throw std::runtime_error("specified declaration is not defined: " + name);
    return 1;
  }

 };
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
                               [](std::string s) { return s; },
                               [](auto v) { return v->toString(); }},
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