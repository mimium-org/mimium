/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/mir.hpp"
#include "basic/variant_visitor_helper.hpp"
#include "compiler/ffi.hpp"
namespace mimium {

class ClosureConverter : public std::enable_shared_from_this<ClosureConverter> {
 public:
  explicit ClosureConverter(TypeEnv& typeenv);
  ~ClosureConverter();
  std::shared_ptr<MIRblock> convert(std::shared_ptr<MIRblock> toplevel);
  void reset();
  bool isKnownFunction(const std::string& name);
  std::string makeCaptureName() {
    return "Capture." + std::to_string(capturecount++);
  }
  std::string makeClosureTypeName() {
    return "Closure." + std::to_string(closurecount++);
  }


 private:
  TypeEnv& typeenv;
  std::shared_ptr<MIRblock> toplevel;
  int capturecount;
  int closurecount;
  std::unordered_map<std::string, int> known_functions;
  FunInst tmp_globalfn;
  void moveFunToTop(std::shared_ptr<MIRblock> mir);

  std::unordered_map<std::string, std::vector<std::string>> fun_to_memory_objs;

  struct CCVisitor {
    explicit CCVisitor(ClosureConverter& cc, std::vector<std::string>& fvlist,
                       std::vector<std::string>& localvlist,
                       std::list<Instructions>::iterator& position)
        : cc(cc), fvlist(fvlist), localvlist(localvlist), position(position) {}

    ClosureConverter& cc;
    std::vector<std::string>& fvlist;
    std::vector<std::string>& localvlist;
    std::list<Instructions>::iterator position;
    void updatepos() { ++position; }
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
    private:
    void visitinsts(FunInst& i,CCVisitor& ccvis,std::list<Instructions>::iterator pos);
  };
  struct FunTypeReplacer {
    explicit FunTypeReplacer(ClosureConverter& cc) : cc(cc) {}

    ClosureConverter& cc;
    bool isClosure(const std::string& name) {
      return (cc.typeenv.tryFind(name + "_cls") != nullptr);
    }
    void replaceType(types::Value& val, const std::string& name) {
      val = cc.typeenv.find(name + "_cls");
      cc.typeenv.emplace(name, val);
    }
    void operator()(NumberInst& i){};
    void operator()(AllocaInst& i) {
      if (isClosure(i.lv_name)) replaceType(i.type, i.lv_name);
    };
    void operator()(RefInst& i) {
      if (isClosure(i.val)) replaceType(i.type, i.val);
    };
    void operator()(AssignInst& i) {
      if (isClosure(i.val)) replaceType(i.type, i.val);
    };
    void operator()(TimeInst& i){};
    void operator()(OpInst& i){};
    void operator()(FunInst& i) {
      for (auto& inst : i.body->instructions) {
        std::visit(*this, inst);
      }
      auto it = i.body->instructions.rbegin();
      auto lastinst = *it;
      if (auto ret = std::get_if<ReturnInst>(&lastinst)) {
        auto& ft = rv::get<types::Function>(i.type);
        ft.ret_type =
            (types::isPrimitive(ret->type)) ? ret->type : types::Ref(ret->type);
      }
      cc.typeenv.emplace(i.lv_name, i.type);
    };
    void operator()(FcallInst& i) {
      if (auto* ftype = std::get_if<Rec_Wrap<types::Function>>(
              &cc.typeenv.find(i.fname))) {
        types::Function& f = ftype->getraw();
        std::optional<types::Value> cls = types::getNamedClosure(f.ret_type);
        if (cls) {
          cc.typeenv.emplace(i.lv_name, std::move(cls.value()));
        }
      }
    };
    void operator()(MakeClosureInst& i){};
    void operator()(ArrayInst& i){};
    void operator()(ArrayAccessInst& i){};
    void operator()(IfInst& i){};
    void operator()(ReturnInst& i) {
      if (isClosure(i.val)) {
        replaceType(i.type, i.val);
      }
    };
  } typereplacer;
};

}  // namespace mimium