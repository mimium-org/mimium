#include "builtin_functions.hpp"

namespace mimium {
mValue builtin::print(std::shared_ptr<ArgumentsAST> argast,
                      Interpreter *interpreter) {
  auto args = argast->getElements();
  for (auto &elem : args) {
    mValue ev = interpreter->interpretExpr(elem);
    std::cout << Interpreter::to_string(ev);
    std::cout << " ";
  }
  return 0.0;
}

mValue builtin::println(std::shared_ptr<ArgumentsAST> argast,
                        Interpreter *interpreter) {
  mimium::builtin::print(argast, interpreter);
  std::cout << std::endl;
  return 0.0;
}

mValue builtin::setMidiOut(std::shared_ptr<ArgumentsAST> argast,
                           Interpreter *interpreter) {
  auto args = argast->getElements();
  double port = Interpreter::get_as_double(
  interpreter->interpretExpr(args[0]));  // ignore multiple arguments
  midi.setPort((int)port);
  midi.printCurrentPort((int)port);
  return 0.0;
}

mValue builtin::sendMidiMessage(std::shared_ptr<ArgumentsAST> argast,
                                Interpreter *interpreter) {
  auto args = argast->getElements();
  mValue val = interpreter->interpretExpr(args[0]);
  std::visit(overloaded{
    [](std::vector<double> vec) {
                          std::vector<unsigned char> outvec;
                          outvec.resize(vec.size());
                          for (int i = 0; i < vec.size(); i++) {
                            outvec[i] = (unsigned char)vec[i];
                          }
                          midi.sendMessage(outvec);
                        },
    [](double d) {
                          std::vector<unsigned char> outvec;
                          outvec.push_back((unsigned char)d);
                          midi.sendMessage(outvec);
                        },
    [](auto v) {
                          throw std::runtime_error("invalid midi message");
                        }},
             val);
  return 0.0;
}

bool builtin::isBuiltin(std::string str) {
  return builtin_fntable.count(str) > 0;
}

}  // namespace mimium