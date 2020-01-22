#pragma once
#include "compiler/ffi.hpp"
#include "basic/mir.hpp"
#include "basic/variant_visitor_helper.hpp"
namespace mimium {


class ClosureConverter : public std::enable_shared_from_this<ClosureConverter> {
 public:
  explicit ClosureConverter(TypeEnv& _typeenv);
  ~ClosureConverter();
  std::shared_ptr<MIRblock> convert(std::shared_ptr<MIRblock> toplevel);
  void reset();
  bool isKnownFunction(const std::string& name);
  TypeEnv& typeenv;
  std::shared_ptr<MIRblock> toplevel;
  int capturecount;
  std::unordered_map<std::string, int> known_functions;
  FunInst tmp_globalfn;

 private:
  void moveFunToTop(std::shared_ptr<MIRblock> mir);

  struct CCVisitor {
    explicit CCVisitor(ClosureConverter& cc, std::vector<std::string>& fvlist,std::vector<std::string>& localvlist,
                       std::list<Instructions>::iterator& position)
        : cc(cc), fvlist(fvlist), localvlist(localvlist),position(position) {}

    ClosureConverter& cc;
    std::vector<std::string>& fvlist;
    std::vector<std::string> &localvlist;
    std::vector<std::string> funlist;

    std::list<Instructions>::iterator position;
    void updatepos(){++position;}
    void registerFv(std::string& name);
    void operator()(NumberInst& i);
    void operator()(AllocaInst& i);
    void operator()(RefInst& i);
    void operator()(AssignInst& i);
    void operator()(TimeInst& i);
    void operator()(OpInst& i);
    void operator()(FunInst& i);
    void operator()(FcallInst& i);
    void operator()(MakeClosureInst& i);
    void operator()(ArrayInst& i);
    void operator()(ArrayAccessInst& i);
    void operator()(IfInst& i);
    void operator()(ReturnInst& i);
    bool isFreeVar(const std::string& name);
  };
};

}  // namespace mimium