#include "builtin_functions.hpp"

namespace mimium {

const std::map<std::string, mmmfn> Builtin::builtin_fntable = {
    {"printchar", &Builtin::print},
    {"print", &Builtin::print},
    {"println", &Builtin::println},
    {"setMidiOut", &Builtin::setMidiOut},
    {"setVirtualMidiOut", &Builtin::setVirtualMidiOut},
    {"sendMidiMessage", &Builtin::sendMidiMessage},
    {"sin", &Builtin::sin},
    {"cos", &Builtin::cos},
};

mValue Builtin::print(std::shared_ptr<ArgumentsAST> argast,
                      InterpreterVisitor *interpreter) {
  auto args = argast->getElements();
  for (auto &elem : args) {
    elem->accept(*interpreter);
    mValue ev = interpreter->getVstack().top();
    interpreter->getVstack().pop();
    std::cout << InterpreterVisitor::to_string(ev);
    std::cout << " ";
  }
  return 0.0;
}

mValue Builtin::println(std::shared_ptr<ArgumentsAST> argast,
                        InterpreterVisitor *interpreter) {
  print(argast, interpreter);
  std::cout << std::endl;
  return 0.0;
}

mValue Builtin::setMidiOut(std::shared_ptr<ArgumentsAST> argast,
                           InterpreterVisitor *interpreter) {
  auto args = argast->getElements();
  args[0]->accept(*interpreter);
  double port = std::get<double>(interpreter->getVstack().top());
  interpreter->getVstack().pop();
  interpreter->getRuntime().getMidiInstance().setPort((int)port);
  interpreter->getRuntime().getMidiInstance().printCurrentPort((int)port);
  return 0.0;
};
mValue Builtin::setVirtualMidiOut(std::shared_ptr<ArgumentsAST> argast,
                                  InterpreterVisitor *interpreter) {
  // ignore arguments
  interpreter->getRuntime().getMidiInstance().createVirtualPort();
  return 0.0;
};
mValue Builtin::sendMidiMessage(std::shared_ptr<ArgumentsAST> argast,
                                InterpreterVisitor *interpreter) {
  auto args = argast->getElements();
  args[0]->accept(*interpreter);
  mValue val = interpreter->getVstack().top();
  interpreter->getVstack().pop();
  auto message = std::visit(
      overloaded{[](std::vector<double> vec) -> std::vector<unsigned char> {
                   std::vector<unsigned char> outvec;
                   outvec.resize(vec.size());
                   for (int i = 0; i < vec.size(); i++) {
                     outvec[i] = (unsigned char)vec[i];
                   }
                   return outvec;
                 },
                 [](double d) -> std::vector<unsigned char> {
                   std::vector<unsigned char> outvec;
                   outvec.push_back((unsigned char)d);
                   return outvec;
                 },
                 [](auto v) -> std::vector<unsigned char> {
                   throw std::runtime_error("invalid midi message");
                   return std::vector<unsigned char>{};
                 }},
      val);
  interpreter->getRuntime().getMidiInstance().sendMessage(message);
  return 0.0;
};

mValue Builtin::cmath(std::function<double(double)> fn,
                      std::shared_ptr<ArgumentsAST> argast,
                      InterpreterVisitor* interpreter) {
  auto args = argast->getElements();
  args[0]->accept(*interpreter);
  auto val = std::get<double>(interpreter->getVstack().top());
  interpreter->getVstack().pop();
  return fn(val);
}

mValue Builtin::sin(std::shared_ptr<ArgumentsAST> argast,
                    InterpreterVisitor *interpreter) {
  return cmath([](double d) -> double { return std::sin(d); }, argast,
               interpreter);
}
mValue Builtin::cos(std::shared_ptr<ArgumentsAST> argast,
                    InterpreterVisitor *interpreter) {
  return cmath([](double d) -> double { return std::cos(d); }, argast,
               interpreter);
}
// const mmmfn Builtin::createMathFn(std::function<double(double)> fn,
//                                   std::shared_ptr<ArgumentsAST> argast,
//                                   InterpreterVisitor *interpreter) {
//     auto args = argast->getElements();
//     auto val =
//     InterpreterVisitor::get_as_double(interpreter->interpretExpr(args[0]));
//     return fn(val);
// }

const bool Builtin::isBuiltin(std::string str) {
  return builtin_fntable.count(str) > 0;
}

}  // namespace mimium