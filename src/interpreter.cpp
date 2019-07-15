#include "interpreter.hpp"
#include <cmath>

namespace mimium {

void Interpreter::init(){
      rootenv = std::make_shared<Environment>("root",nullptr);
      currentenv = rootenv; // share
}
void Interpreter::clear() { 
      rootenv.reset();
      currentenv.reset();
      driver.clear();
      init();
} 

void Interpreter::start() { sch->start(); }

void Interpreter::stop() { sch->stop(); }

void Interpreter::loadSource(const std::string src) {
  driver.parsestring(src);
  loadAst(driver.getMainAst());
}
void Interpreter::loadSourceFile(const std::string filename) {
  driver.parsefile(filename);
  loadAst(driver.getMainAst());
}

mValue Interpreter::loadAst(AST_Ptr _ast) {
  topast = _ast;
  return interpretTopAst();
}

mValue Interpreter::interpretListAst(AST_Ptr ast) {
  try {
    mValue res;
    switch (ast->getid()) {
      case LIST:
        for (auto& line : std::dynamic_pointer_cast<ListAST>(ast)->getlist()) {
          res = interpretListAst(line);
          if (line->getid() == RETURN) break;
        }
        break;
      default:
        res = interpretStatementsAst(ast);
        break;
    }
    return res;
  } catch (std::exception e) {
    std::cerr << e.what() << std::endl;
    return 0.0;
  }
}

mValue Interpreter::interpretStatementsAst(AST_Ptr line) {
  mValue tmpres = false;

  switch (line->getid()) {
    case ASSIGN:
      tmpres = interpretAssign(line);
      break;
    // fdef is directly converted into assign lambda
    // case FDEF:
    //     tmpres=  interpretFdef(line);
    //     break;
    case IF:
      tmpres = interpretIf(line);
      break;
    case FOR:
      tmpres = interpretFor(line);
      break;
    case DECLARATION:
      tmpres = interpretDeclaration(line);
      break;
    case RETURN:
      tmpres = interpretReturn(line);
      goto end;
    default:
      tmpres = interpretExpr(line);
      break;
  }
end:
  return tmpres;
}

mValue Interpreter::interpretReturn(AST_Ptr line) {
  auto ret = std::dynamic_pointer_cast<ReturnAST>(line);
  return interpretExpr(ret->getExpr());
}

mValue Interpreter::interpretAssign(AST_Ptr line) {
  auto assign = std::dynamic_pointer_cast<AssignAST>(line);
  std::string varname = assign->getName()->getVal();
  if (currentenv->isVariableSet(varname)) {
    // std::cout<<"Variable "<< varname << " already exists.
    // Overwritten"<<std::endl;
  }
  auto body = assign->getBody();
  if (body) {
    mValue res = interpretExpr(body);
    currentenv->setVariable(varname, res);  // share
    std::cout << "Variable " << varname << " : " << Interpreter::to_string(res)
              << std::endl;
    return line;  // for print
  } else {
    throw std::runtime_error("expression not resolved");
  }
}
mValue Interpreter::interpretDeclaration(AST_Ptr line) {
  auto fcall = std::dynamic_pointer_cast<DeclarationAST>(line);
  auto name = std::dynamic_pointer_cast<SymbolAST>(fcall->getFname())->getVal();
  auto argsast = fcall->getArgs();
  auto args = argsast->getElements();
  if (name == "include") {
    assertArgumentsLength(args, 1);
    if (args[0]->getid() == SYMBOL) {
      auto filename = std::dynamic_pointer_cast<SymbolAST>(args[0])->getVal();
      auto temporary_driver = std::make_unique<mmmpsr::MimiumDriver>(current_working_directory);
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
// namespace mimium
// mValue Interpreter::interpretFdef(AST_Ptr line){
//     try{
//         auto fdef = std::dynamic_pointer_cast<FdefAST>(line);
//         std::string fname = fdef->getFname()->getVal();
//         currentenv = currentenv->createNewChild(fname); //switch
//         mValue interres = interpretStatementsAst(fdef->getFbody());

//     }catch(std::exception e){
//         std::cerr<<e.what()<<std::endl;
//         return false;
//     }
// }

mValue Interpreter::interpretExpr(AST_Ptr expr) {
  switch (expr->getid()) {
    case SYMBOL:
      return interpretVariable(expr);
      break;
    case NUMBER:
      return interpretNumber(expr);
      break;
    case OP:
      return interpretBinaryExpr(expr);
      break;
    case LAMBDA:
      return interpretLambda(expr);
      break;
    case FCALL:
      return interpretFcall(expr);
      break;
    case ARRAY:
      return interpretArray(expr);
      break;
    case ARRAYACCESS:
      return interpretArrayAccess(expr);
      break;
    case TIME:
      return interpretTime(expr);
      break;
    default:
      throw std::runtime_error("invalid expression");
      return 0.0;
  }
}
overloaded binary_visitor{
    [](double lhs) { return lhs; },
    [](auto lhs) {
      mValue val = (lhs);
      throw std::runtime_error("invalid binary expression");
      return 0.0;
    }};
mValue Interpreter::interpretBinaryExpr(AST_Ptr expr) {
  auto var = std::dynamic_pointer_cast<OpAST>(expr);
  mValue lhs = interpretExpr(var->lhs);
  double lv = std::visit(binary_visitor, lhs);
  mValue rhs = interpretExpr(var->rhs);
  double rv = std::visit(binary_visitor, rhs);

  switch (var->getOpId()) {
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

mValue Interpreter::interpretVariable(AST_Ptr symbol) {
  auto var = std::dynamic_pointer_cast<SymbolAST>(symbol);
  return currentenv->findVariable(var->getVal());
}

mValue Interpreter::interpretNumber(AST_Ptr num) {
  auto var = std::dynamic_pointer_cast<NumberAST>(num);
  return var->getVal();
}
mValue Interpreter::interpretLambda(AST_Ptr expr) {
  auto lambda = std::dynamic_pointer_cast<LambdaAST>(expr);
  auto closure = std::make_shared<Closure>(currentenv, lambda);
  return std::move(closure);
}
overloaded fcall_visitor{
    [](auto v) -> mClosure_ptr {
      throw std::runtime_error("reffered variable is not a function");
      return nullptr;
    },
    [](std::shared_ptr<Closure> v) -> mClosure_ptr { return v; }};
mValue Interpreter::interpretFcall(AST_Ptr expr) {
  auto fcall = std::dynamic_pointer_cast<FcallAST>(expr);
  auto name = std::dynamic_pointer_cast<SymbolAST>(fcall->getFname())->getVal();
  auto args = fcall->getArgs();
  if (mimium::builtin::isBuiltin(name)) {
    auto fn = mimium::builtin::builtin_fntable.at(name);
    fn(args, this);  // currently implemented only for print()
    return 0.0;
  } else {
    auto argsv = args->getElements();
    mValue var = findVariable(name);
    mClosure_ptr closure = std::visit(fcall_visitor, var);
    auto lambda = closure->fun;
    std::shared_ptr<Environment> originalenv = currentenv;
    std::shared_ptr<Environment> tmpenv = closure->env;
    auto lambdaargs = std::dynamic_pointer_cast<ArgumentsAST>(lambda->getArgs())
                          ->getElements();
    auto body = lambda->getBody();
    tmpenv = tmpenv->createNewChild(name);  // create arguments
    int argscond = lambdaargs.size() - argsv.size();
    if (argscond < 0) {
      throw std::runtime_error("too many arguments");
    } else {
      int count = 0;
      for (auto& larg : lambdaargs) {
        std::string key = std::dynamic_pointer_cast<SymbolAST>(larg)->getVal();
        // arguments[key]=interpretExpr(argsv[count]);
        tmpenv->getVariables()[key] = interpretExpr(argsv[count]);
        count++;
      }
      if (argscond == 0) {
        currentenv = tmpenv;  // switch back env
        auto tmp = lambda->getBody();
        auto res = interpretListAst(tmp);
        currentenv = originalenv;
        return res;
      } else {
        throw std::runtime_error(
            "too few arguments");  // ideally we want to return new function
                                   // (partial application)
      }
    }
  }
}
mValue Interpreter::interpretArray(AST_Ptr array) {
  auto arr = std::dynamic_pointer_cast<ArrayAST>(array);
  std::vector<double> v;
  for (auto& elem : arr->getElements()) {
    double res = get_as_double(interpretExpr(elem));
    v.push_back(res);
  }
  return std::move(v);
}
mValue Interpreter::interpretArrayAccess(AST_Ptr arrayast) {
  auto arr = std::dynamic_pointer_cast<ArrayAccessAST>(arrayast);
  auto array = interpretVariable(arr->getName());
  auto index = get_as_double(interpretExpr(arr->getIndex()));
  int i = (int)index;  // now index is simply casted from double
  return std::visit(
      overloaded{[&i](std::vector<double> a) -> double { return a[i]; },
                 [](auto e) -> double {
                   throw std::runtime_error(
                       "accessed variable is not an array");
                   return 0;
                 }},
      array);
}

mValue Interpreter::interpretIf(AST_Ptr expr) {
  auto ifexpr = std::dynamic_pointer_cast<IfAST>(expr);
  mValue cond = interpretExpr(ifexpr->getCond());
  auto cond_d = get_as_double(cond);
  if (cond_d > 0) {
    return interpretListAst(ifexpr->getThen());
  } else {
    return interpretListAst(ifexpr->getElse());
  }
}

mValue Interpreter::interpretFor(AST_Ptr expr) {
  auto forexpr = std::dynamic_pointer_cast<ForAST>(expr);
  std::string loopname = "for" + std::to_string(rand());  // temporary,,
  currentenv = currentenv->createNewChild(loopname);
  std::string varname =
      std::dynamic_pointer_cast<SymbolAST>(forexpr->getVar())->getVal();
  AST_Ptr expression = forexpr->getExpression();
  mValue iterator = interpretExpr(forexpr->getIterator());
  std::visit(overloaded{[&](std::vector<double> v) {
                          auto it = v.begin();
                          while (it != v.end()) {
                            currentenv->setVariable(varname, *it);
                            interpretListAst(expression);
                            it++;
                          }
                        },
                        [&](double v) {
                          currentenv->setVariable(varname, v);
                          interpretListAst(expression);
                        },
                        [](auto v) {
                          throw std::runtime_error("iterator is invalid");
                        }},
             iterator);
  currentenv = currentenv->getParent();
  return 0.0;  // forloop does not return value
}

mValue Interpreter::interpretTime(AST_Ptr expr) {
  auto timeexpr = std::dynamic_pointer_cast<TimeAST>(expr);
  mValue time = interpretExpr(timeexpr->getTime());
  std::visit(overloaded{[&](double t) { sch->addTask(t, timeexpr->getExpr()); },
                        [](auto t) {
                          throw std::runtime_error(
                              "you cannot append value other than double");
                        }},
             time);
  return 0;
}

double Interpreter::get_as_double(mValue v) {
  return std::visit(
      overloaded{[](double v) -> double { return v; },
                 [](auto v) -> double {
                   throw std::runtime_error("value is not double");
                   return 0;
                 }},
      v);
};

std::string Interpreter::to_string(mValue v) {
  return std::visit(overloaded{[](double v) { return std::to_string(v); },
                               [](AST_Ptr v) {
                                 std::stringstream ss;
                                 v->to_string(ss);
                                 return ss.str();
                               },
                               [](mClosure_ptr v) { return v->to_string(); },
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
                               }},
                    v);
};

bool Interpreter::assertArgumentsLength(std::vector<AST_Ptr>& args,
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