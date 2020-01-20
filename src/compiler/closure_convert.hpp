#pragma once
#include "basic/environment.hpp"
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
  std::unordered_map<std::string, std::shared_ptr<FunInst>> known_functions;

 private:
  void moveFunToTop(std::shared_ptr<MIRblock> mir);

  struct CCVisitor {
    explicit CCVisitor(ClosureConverter& cc, std::vector<std::string>& fvlist,
                       std::list<Instructions>::iterator position)
        : cc(cc), fvlist(fvlist), position(position) {}

    ClosureConverter& cc;
    std::vector<std::string>& fvlist;
    std::vector<std::string> localvlist;
    std::list<Instructions>::iterator position;
    void operator()(std::shared_ptr<NumberInst> i);
    void operator()(std::shared_ptr<AllocaInst> i);
    void operator()(std::shared_ptr<RefInst> i);
    void operator()(std::shared_ptr<AssignInst> i);
    void operator()(std::shared_ptr<TimeInst> i);
    void operator()(std::shared_ptr<OpInst> i);
    void operator()(std::shared_ptr<FunInst> i);
    void operator()(std::shared_ptr<FcallInst> i);
    void operator()(std::shared_ptr<MakeClosureInst> i);
    void operator()(std::shared_ptr<ArrayInst> i);
    void operator()(std::shared_ptr<ArrayAccessInst> i);
    void operator()(std::shared_ptr<IfInst> i);
    void operator()(std::shared_ptr<ReturnInst> i);
    bool isFreeVar(const std::string& name);
  };
};

}  // namespace mimium