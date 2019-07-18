#include "builtin_functions.hpp"

namespace mimium {

const std::map<std::string, mmmfn> Builtin::builtin_fntable = {
    {"printchar", &Builtin::print},
    {"print", &Builtin::print},
    {"println", &Builtin::println},
    {"setMidiOut", &Builtin::setMidiOut},
    {"sendMidiMessage", &Builtin::sendMidiMessage},
    {"sin", &Builtin::sin},
    {"cos", &Builtin::cos},
};


mValue Builtin::print(std::shared_ptr<ArgumentsAST> argast,
                      Interpreter *interpreter) {
  auto args = argast->getElements();
  for (auto &elem : args) {
    mValue ev = interpreter->interpretExpr(elem);
    std::cout << Interpreter::to_string(ev);
    std::cout << " ";
  }
  return 0.0;
}

mValue Builtin::println(std::shared_ptr<ArgumentsAST> argast,
                        Interpreter *interpreter) {
  print(argast, interpreter);
  std::cout << std::endl;
  return 0.0;
}

mValue Builtin::setMidiOut(std::shared_ptr<ArgumentsAST> argast,
                           Interpreter *interpreter) {
  auto args = argast->getElements();
  double port = Interpreter::get_as_double(
      interpreter->interpretExpr(args[0]));  // ignore multiple arguments
  interpreter->getMidiInstance().setPort((int)port);
  interpreter->getMidiInstance().printCurrentPort((int)port);
  return 0.0;
};

mValue Builtin::sendMidiMessage(std::shared_ptr<ArgumentsAST> argast,
                                Interpreter *interpreter) {
  auto args = argast->getElements();
  mValue val = interpreter->interpretExpr(args[0]);
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
  interpreter->getMidiInstance().sendMessage(message);
  return 0.0;
};

mValue Builtin::cmath(std::function<double(double)> fn,
                      std::shared_ptr<ArgumentsAST> argast,
                      Interpreter *interpreter) {
  auto args = argast->getElements();
  auto val = Interpreter::get_as_double(interpreter->interpretExpr(args[0]));
  return fn(val);
}

mValue Builtin::sin(std::shared_ptr<ArgumentsAST> argast,
                    Interpreter *interpreter) {
  return cmath([](double d) -> double { return std::sin(d); }, argast,
               interpreter);
}
mValue Builtin::cos(std::shared_ptr<ArgumentsAST> argast,
                    Interpreter *interpreter) {
  return cmath([](double d) -> double { return std::cos(d); }, argast,
               interpreter);
}
// const mmmfn Builtin::createMathFn(std::function<double(double)> fn,
//                                   std::shared_ptr<ArgumentsAST> argast,
//                                   Interpreter *interpreter) {
//     auto args = argast->getElements();
//     auto val =
//     Interpreter::get_as_double(interpreter->interpretExpr(args[0])); return
//     fn(val);
// }

const bool Builtin::isBuiltin(std::string str) {
  return builtin_fntable.count(str) > 0;
}

}  // namespace mimium