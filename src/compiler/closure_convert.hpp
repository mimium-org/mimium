#pragma once
#include "basic/mir.hpp"
#include "basic/variant_visitor_helper.hpp"
#include "compiler/ffi.hpp"
namespace mimium {

class ClosureConverter : public std::enable_shared_from_this<ClosureConverter> {
 public:
  explicit ClosureConverter(TypeEnv& _typeenv);
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
  TypeEnv& typeenv;
  std::shared_ptr<MIRblock> toplevel;
  int capturecount;
  int closurecount;
  std::unordered_map<std::string, int> known_functions;
  std::unordered_map<std::string, types::Closure> funtoclsmap;
  FunInst tmp_globalfn;
  
 private:
  void moveFunToTop(std::shared_ptr<MIRblock> mir);
  struct CCVisitor {
    explicit CCVisitor(
        ClosureConverter& cc, std::vector<std::string>& fvlist,
        std::vector<std::string>& localvlist,
        std::unordered_map<std::string, types::Closure>& funtoclsmap,
        std::list<Instructions>::iterator& position)
        : cc(cc),
          fvlist(fvlist),
          localvlist(localvlist),
          funtoclsmap(funtoclsmap),
          position(position) {}

    ClosureConverter& cc;
    std::vector<std::string>& fvlist;
    std::vector<std::string>& localvlist;
    std::unordered_map<std::string, types::Closure>& funtoclsmap;
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
  };
  struct FunTypeReplacer {
    explicit FunTypeReplacer(ClosureConverter& cc)
        : cc(cc){}

    ClosureConverter& cc;
    bool isClosure(std::string& name){
      return (cc.typeenv.tryFind(name+"_cls")!=nullptr);
    }
    void replaceType(types::Value& val,const std::string& name){
      val = cc.typeenv.find(name+"_cls");
      cc.typeenv.emplace(name, val);
    }
    void operator()(NumberInst& i){};
    void operator()(AllocaInst& i){
          if(isClosure(i.lv_name))replaceType(i.type,i.lv_name);
    };
    void operator()(RefInst& i){
      if(isClosure(i.val))replaceType(i.type,i.val);
    };
    void operator()(AssignInst& i){
      if(isClosure(i.val))replaceType(i.type,i.val);
    };
    void operator()(TimeInst& i){};
    void operator()(OpInst& i){};
    void operator()(FunInst& i){
      for(auto&inst: i.body->instructions ){
          std::visit(*this,inst);
      }
      auto it = i.body->instructions.rbegin();
      auto lastinst = *it; 
      if(auto ret =std::get_if<ReturnInst>(&lastinst)){
        auto& ft = std::get<recursive_wrapper<types::Function>>(i.type).getraw();
        ft.ret_type = (types::isPrimitive(ret->type))?ret->type:types::Ref(ret->type);
      }
      cc.typeenv.emplace(i.lv_name, i.type);

    };
    void operator()(FcallInst& i){
      if(auto* ftype = std::get_if<recursive_wrapper<types::Function>>(&cc.typeenv.find(i.fname))){
          types::Function& f  = ftype->getraw();
          std::optional<types::Value> cls = types::getNamedClosure(f.ret_type);
          if(cls){
            cc.typeenv.emplace(i.lv_name, std::move(cls.value()));
          }
      }
    };
    void operator()(MakeClosureInst& i){};
    void operator()(ArrayInst& i){
    };
    void operator()(ArrayAccessInst& i){};
    void operator()(IfInst& i){};
    void operator()(ReturnInst& i){
      if(isClosure(i.val)){replaceType(i.type,i.val);}

    };
  }typereplacer;
};

}  // namespace mimium