#include "variant_visitor_helper.hpp"
#include "mir.hpp"
namespace mimium {

class ClosureConverter {
 public:
  ClosureConverter();
  ~ClosureConverter();
  std::shared_ptr<MIRblock> convert(std::shared_ptr<MIRblock> mir);

  private:
  std::unordered_map<std::string,std::shared_ptr<FunInst>> closure_functions;
  // std::unordered_map<std::string,std::deque<std::string>> fname_to_freevars;
  bool isClosureFunction(std::string str);
  void movetoTopLevel_Closure();
  void convertFun(std::shared_ptr<FunInst> inst);
  void insertMakeClosure(std::shared_ptr<MIRblock> mir,std::deque<Instructions>::iterator it,std::shared_ptr<FcallInst> inst);
  int capturecount;
};

}  // namespace mimium