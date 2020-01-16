#pragma once
#include "basic/environment.hpp"
#include "basic/mir.hpp"
#include "basic/variant_visitor_helper.hpp"
namespace mimium {

class NumberInst;
class AllocaInst;
class RefInst;
class AssignInst;
class TimeInst;
class OpInst;
class FunInst;
class FcallInst;
class MakeClosureInst;
class ArrayInst;
class ArrayAccessInst;
class IfInst;
class ReturnInst;

using Instructions =
    std::variant<std::shared_ptr<NumberInst>, std::shared_ptr<AllocaInst>,
                 std::shared_ptr<RefInst>, std::shared_ptr<AssignInst>,std::shared_ptr<TimeInst>,
                 std::shared_ptr<OpInst>, std::shared_ptr<FunInst>,
                 std::shared_ptr<FcallInst>, std::shared_ptr<MakeClosureInst>,
                 std::shared_ptr<ArrayInst>, std::shared_ptr<ArrayAccessInst>,
                 std::shared_ptr<IfInst>, std::shared_ptr<ReturnInst> >;
class MIRblock;

class ClosureConverter : public std::enable_shared_from_this<ClosureConverter> {
 public:
  explicit ClosureConverter(TypeEnv& _typeenv);
  ~ClosureConverter();
  std::shared_ptr<MIRblock> convert(std::shared_ptr<MIRblock> mir);
  void reset();
  TypeEnv& typeenv;
  std::shared_ptr<MIRblock> toplevel;
  std::shared_ptr<Environment<std::string>> env;
  int capturecount;
  std::unordered_map<std::string, std::shared_ptr<FunInst>> known_functions;

 private:
  std::shared_ptr<MIRblock> convertRaw(std::shared_ptr<MIRblock> mir);
  std::shared_ptr<MIRblock> moveFunToTop(std::shared_ptr<MIRblock> mir);

  std::unordered_map<std::string, std::shared_ptr<FunInst>> toplevel_functions;
};
}  // namespace mimium