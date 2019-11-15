#pragma once
#include "variant_visitor_helper.hpp"
#include "mir.hpp"
#include "environment.hpp"
namespace mimium {

class NumberInst;
class SymbolInst;
class RefInst;
class TimeInst;
class OpInst;
class FunInst;
class FcallInst;
class MakeClosureInst;
class ArrayInst;
class ArrayAccessInst;
class IfInst;
class ReturnInst;

using Instructions = std::variant<std::shared_ptr<NumberInst>,std::shared_ptr<SymbolInst>,std::shared_ptr<RefInst>,std::shared_ptr<TimeInst>,std::shared_ptr<OpInst>,std::shared_ptr<FunInst>,std::shared_ptr<FcallInst>,std::shared_ptr<MakeClosureInst>,std::shared_ptr<ArrayInst>,std::shared_ptr<ArrayAccessInst>,std::shared_ptr<IfInst>,std::shared_ptr<ReturnInst>>;
class MIRblock;

class ClosureConverter :public std::enable_shared_from_this<ClosureConverter>{
 public:
  ClosureConverter();
  ~ClosureConverter();
  std::shared_ptr<MIRblock> convert(std::shared_ptr<MIRblock> mir);

  std::shared_ptr<MIRblock> toplevel;
  std::shared_ptr<Environment> env;
  int capturecount;
  std::unordered_map<std::string,std::shared_ptr<FunInst>> known_functions;

  private:
  std::unordered_map<std::string,std::shared_ptr<FunInst>> toplevel_functions;

  // std::unordered_map<std::string,std::deque<std::string>> fname_to_freevars;
//   bool isClosureFunction(std::string str);
//   void movetoTopLevel_Closure();
//   void convertFun(std::shared_ptr<FunInst> inst);
//   void insertMakeClosure(std::shared_ptr<MIRblock> mir,std::deque<Instructions>::iterator it,std::shared_ptr<FcallInst> inst);
};

}  // namespace mimium