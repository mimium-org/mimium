#pragma once
#include <list>
#include <utility>
#include "ast.hpp"
#include "environment.hpp"
#include "closure_convert.hpp"

namespace mimium {
enum FCALLTYPE { DIRECT, CLOSURE, EXTERNAL };
static std::map<FCALLTYPE, std::string> fcalltype_str={
    {DIRECT , ""}, {CLOSURE , "cls"}, {EXTERNAL , "ext"}};
class ClosureConverter;//forward decl
class MIRblock;
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

struct TypedVal{
  types::Value type;
  std::string name; 
};

class MIRinstruction{  // base class for MIR instruction
 protected:
  virtual ~MIRinstruction()=default;
  bool isFreeVariable(std::shared_ptr<Environment> env,std::string str);
  void gatherFV_raw(std::deque<TypedVal>& fvlist,std::shared_ptr<Environment> env,TypeEnv& typeenv,std::string& str);
 public:
  std::string lv_name;
  types::Value type;
  virtual std::string toString() = 0;
  virtual void closureConvert(std::deque<TypedVal>& fvlist, std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it)=0;
  virtual void moveFunToTop(std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it){/*do nothing other than FunInst*/};
  virtual bool isFunction(){return false;}
};


class MIRblock {
 public:
  explicit MIRblock(std::string _label) : label(std::move(_label)), prev(nullptr), next(nullptr){
    indent_level = 0;
  };
  ~MIRblock(){};
  void addInst(Instructions& inst) {
    instructions.push_back(inst);
  }
  void changeIndent(int level){
    indent_level += level;
  }
  std::string label;
  std::list<Instructions>
      instructions;  // sequence of instructions
  std::shared_ptr<MIRblock> prev;
  std::shared_ptr<MIRblock> next;
  std::string toString();
  int indent_level;//shared between instances
};



class NumberInst : public MIRinstruction {

 public:
  NumberInst(std::string _lv, double _val)
      : val(std::move(_val)) {
        lv_name = _lv;
        type = types::Float();
      }
  double val;
  std::string toString() override;
  void closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it) override;
};
class SymbolInst : public MIRinstruction {  //unused??

  std::string val;

 public:
  std::string toString() override;
  void closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it) override;

};
class RefInst : public MIRinstruction{
  std::string val;
  public:
  RefInst(std::string _lv, std::string _val, types::Value _type = types::Float() )
      : val(std::move(_val)) {
        lv_name = _lv;
        type = _type;
  }
  std::string toString() override;
  void closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it) override;

};

class TimeInst: public MIRinstruction{
  public:
  std::string time;
  std::string val;
  TimeInst(std::string _lv, std::string _val,std::string _time):time(std::move(_time)),val(std::move(_val)){
    lv_name=_lv;
    type = types::Time();
  }
  std::string toString() override;
  void closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it) override;

};
class OpInst : public MIRinstruction {
 public:
  std::string op;
  std::string lhs;
  std::string rhs;
  OpInst(std::string _lv, std::string _op, std::string _lhs, std::string _rhs)
      : op(std::move(_op)),
        lhs(std::move(_lhs)),
        rhs(std::move(_rhs)){
    lv_name=_lv;
    type = types::Float();
        };
  std::string toString() override;
  OP_ID getOPid(){return optable[op];}
  void closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it) override;

  ~OpInst(){};
};



class FunInst : public MIRinstruction ,public std::enable_shared_from_this<FunInst> {
 public:
  std::deque<std::string> args;
  std::shared_ptr<MIRblock> body;
  std::deque<TypedVal> freevariables; //introduced in closure conversion;
  FunInst(std::string name,std::deque<std::string> newargs,types::Value _type = types::Void()):args(std::move(newargs)) {
    body = std::make_shared<MIRblock>(name);
    lv_name = name; 
    type = _type;//temporary;
    };
  std::string toString() override;
  void closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it) override;
  void moveFunToTop(std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it) override;
  bool isFunction() override {return true;}
  static types::Struct getFvType(std::deque<TypedVal>& fvlist);

};
class FcallInst : public MIRinstruction {
 public:
  std::string fname;
  std::deque<std::string> args;
  FCALLTYPE ftype;
  FcallInst(std::string _lv, std::string _fname, std::deque<std::string> _args,FCALLTYPE _ftype = CLOSURE,types::Value _type = types::Float())
      :fname(std::move(_fname)), args(std::move(_args)),ftype(_ftype){
        lv_name=_lv;
        type = _type;
      };
  std::string toString() override;
  void closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it) override;

};
class MakeClosureInst : public MIRinstruction {
 public:
   std::string fname;
  std::deque<TypedVal> captures;
  std::string toString() override;
  MakeClosureInst(std::string _lv,std::string _fname,std::deque<TypedVal>  _captures,types::Value _type):fname(std::move(_fname)),captures(std::move(_captures)){
            lv_name=_lv;
            type = _type;
  };
  void closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it) override;

};
class ArrayInst : public MIRinstruction {
  std::string name;
  std::deque<std::string> args;
 public:
   ArrayInst(std::string _lv,std::deque<std::string> _args):args(std::move(_args))
   {
     lv_name=_lv;
     type = types::Array(types::Float());
   }

  std::string toString() override;
  void closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it) override;

};
class ArrayAccessInst : public MIRinstruction {
  std::string name;
  std::string index;

 public:
  ArrayAccessInst(std::string _lv,std::string _name,std::string _index):name(std::move(_name)),index(std::move(_index)){
    lv_name = _lv;
    type = types::Float();
  }
  std::string toString() override;
  void closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it) override;

};
class IfInst : public MIRinstruction {
 public:
  std::string cond;
  std::shared_ptr<MIRblock> thenblock;
  std::shared_ptr<MIRblock> elseblock;
  IfInst(std::string name, std::string _cond):cond(std::move(_cond)) {
    thenblock = std::make_shared<MIRblock>(name + "$then");
    elseblock = std::make_shared<MIRblock>(name + "$else");
    lv_name=name;
    type = types::Void();
  }
  std::string toString() override;
  void closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it) override;

};
class ReturnInst: public MIRinstruction{
    public:
    std::string val;
    ReturnInst(std::string name,std::string _val,types::Value _type = types::Float()):val(std::move(_val)){
      lv_name = name;
      type = _type;
    }
    std::string toString() override;
    void closureConvert(std::deque<TypedVal>& fvlist,std::shared_ptr<ClosureConverter> cc ,std::shared_ptr<MIRblock> mir,std::list<Instructions>::iterator it) override;

};
}  // namespace mimium