#pragma once
#include <list>
#include <utility>

#include "basic/ast.hpp"
#include "basic/environment.hpp"
#include "compiler/closure_convert.hpp"
#include "basic/type.hpp"

namespace mimium {

enum FCALLTYPE { DIRECT, CLOSURE, EXTERNAL };
static std::map<FCALLTYPE, std::string> fcalltype_str = {
    {DIRECT, ""}, {CLOSURE, "cls"}, {EXTERNAL, "ext"}};

class ClosureConverter;  // forward decl
class MIRblock;

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
                 std::shared_ptr<RefInst>, std::shared_ptr<AssignInst>,
                 std::shared_ptr<TimeInst>, std::shared_ptr<OpInst>,
                 std::shared_ptr<FunInst>, std::shared_ptr<FcallInst>,
                 std::shared_ptr<MakeClosureInst>, std::shared_ptr<ArrayInst>,
                 std::shared_ptr<ArrayAccessInst>, std::shared_ptr<IfInst>,
                 std::shared_ptr<ReturnInst> >;

struct TypedVal {
  types::Value type;
  std::string name;
};
[[maybe_unused]] static std::string join(const std::vector<TypedVal>& vec,
                                         const std::string& delim) {
  return std::accumulate(std::next(vec.begin()), vec.end(),
                         vec.begin()->name,
                         [&](const std::string& a, const TypedVal& b) {
                           return a + delim + b.name;
                         });
};

using SymbolEnv = Environment<std::string>;

class MIRinstruction {  // base class for MIR instruction
 protected:
  virtual ~MIRinstruction() = default;
  bool isFreeVariable(std::shared_ptr<SymbolEnv> env, std::string str);
  bool gatherFV_raw(std::vector<TypedVal>& fvlist,
                    std::shared_ptr<SymbolEnv> env, TypeEnv& typeenv,
                    std::string& str, std::string& parent_name);
  void checkLvalue(std::vector<TypedVal>& fvlist,
                   std::shared_ptr<ClosureConverter> cc,
                   std::string& parent_name);

 public:
  std::string lv_name;
  types::Value type;
  MIRinstruction() = default;
  MIRinstruction(std::string lv_name, types::Value type = types::None())
      : lv_name(std::move(lv_name)), type(std::move(type)) {}
  virtual std::string toString() = 0;
  virtual void closureConvert(std::vector<TypedVal>& fvlist,
                              std::shared_ptr<ClosureConverter> cc,
                              std::shared_ptr<MIRblock> mir,
                              std::list<Instructions>::iterator it) = 0;
  virtual void moveFunToTop(std::shared_ptr<ClosureConverter> cc,
                            std::shared_ptr<MIRblock> mir,
                            std::list<Instructions>::iterator it){
      /*do nothing other than FunInst*/};
  virtual bool isFunction() { return false; }
};

class MIRblock {
 public:
  explicit MIRblock(std::string _label)
      : label(std::move(_label)), prev(nullptr), next(nullptr) {
    indent_level = 0;
  };
  ~MIRblock(){};
  void addInst(Instructions& inst) { instructions.push_back(inst); }
  void changeIndent(int level) { indent_level += level; }
  std::string label;
  std::list<Instructions> instructions;  // sequence of instructions
  std::shared_ptr<MIRblock> prev;
  std::shared_ptr<MIRblock> next;
  std::string toString();
  int indent_level;  // shared between instances
};

class NumberInst : public MIRinstruction {
 public:
  NumberInst(std::string _lv, double _val)
      : MIRinstruction(_lv, types::Float()), val(std::move(_val)) {}
  double val;
  std::string toString() override;
  void closureConvert(std::vector<TypedVal>& fvlist,
                      std::shared_ptr<ClosureConverter> cc,
                      std::shared_ptr<MIRblock> mir,
                      std::list<Instructions>::iterator it) override;
};
class AllocaInst : public MIRinstruction {  // unused??

 public:
  AllocaInst(std::string lv, types::Value type = types::Float())
      : MIRinstruction(lv, type) {}
  std::string toString() override;
  void closureConvert(std::vector<TypedVal>& fvlist,
                      std::shared_ptr<ClosureConverter> cc,
                      std::shared_ptr<MIRblock> mir,
                      std::list<Instructions>::iterator it) override;
};
class RefInst : public MIRinstruction {
 public:
  std::string val;
  RefInst(std::string _lv, std::string _val,
          types::Value _type = types::Float())
      : MIRinstruction(_lv, _type), val(std::move(_val)) {}
  std::string toString() override;
  void closureConvert(std::vector<TypedVal>& fvlist,
                      std::shared_ptr<ClosureConverter> cc,
                      std::shared_ptr<MIRblock> mir,
                      std::list<Instructions>::iterator it) override;
};
class AssignInst : public MIRinstruction {
 public:
  std::string val;

  AssignInst(std::string _lv, std::string _val,
             types::Value _type = types::Float())
      : MIRinstruction(_lv, _type), val(std::move(_val)) {}
  std::string toString() override;
  void closureConvert(std::vector<TypedVal>& fvlist,
                      std::shared_ptr<ClosureConverter> cc,
                      std::shared_ptr<MIRblock> mir,
                      std::list<Instructions>::iterator it) override;
};

class TimeInst : public MIRinstruction {
 public:
  std::string time;
  std::string val;
  TimeInst(std::string _lv, std::string _val, std::string _time,
           types::Value _type)
      : MIRinstruction(_lv, _type),
        time(std::move(_time)),
        val(std::move(_val)) {}
  std::string toString() override;
  void closureConvert(std::vector<TypedVal>& fvlist,
                      std::shared_ptr<ClosureConverter> cc,
                      std::shared_ptr<MIRblock> mir,
                      std::list<Instructions>::iterator it) override;
};
class OpInst : public MIRinstruction {
 public:
  std::string op;
  std::string lhs;
  std::string rhs;
  OpInst(std::string _lv, std::string _op, std::string _lhs, std::string _rhs)
      : MIRinstruction(_lv, types::Float()),
        op(std::move(_op)),
        lhs(std::move(_lhs)),
        rhs(std::move(_rhs)){};
  std::string toString() override;
  OP_ID getOPid() { return optable[op]; }
  void closureConvert(std::vector<TypedVal>& fvlist,
                      std::shared_ptr<ClosureConverter> cc,
                      std::shared_ptr<MIRblock> mir,
                      std::list<Instructions>::iterator it) override;

  ~OpInst(){};
};

class FunInst : public MIRinstruction,
                public std::enable_shared_from_this<FunInst> {
 public:
  std::deque<std::string> args;
  std::shared_ptr<MIRblock> body;
  std::vector<TypedVal> freevariables;  // introduced in closure conversion;
  bool isrecursive;
  FunInst(std::string name, std::deque<std::string> newargs,
          types::Value _type = types::Void(), bool isrecursive = false)
      : MIRinstruction(name, _type),
        args(std::move(newargs)),
        isrecursive(isrecursive) {
    body = std::make_shared<MIRblock>(name);
  };
  std::string toString() override;
  void closureConvert(std::vector<TypedVal>& fvlist,
                      std::shared_ptr<ClosureConverter> cc,
                      std::shared_ptr<MIRblock> mir,
                      std::list<Instructions>::iterator it) override;
  void closureConvertRaw(std::shared_ptr<ClosureConverter> cc);
  void moveFunToTop(std::shared_ptr<ClosureConverter> cc,
                    std::shared_ptr<MIRblock> mir,
                    std::list<Instructions>::iterator it) override;
  bool isFunction() override { return true; }
  static types::Ref getFvType(std::vector<TypedVal>& fvlist);
};
class FcallInst : public MIRinstruction {
 public:
  std::string fname;
  std::deque<std::string> args;
  FCALLTYPE ftype;
  bool istimed;
  FcallInst(std::string _lv, std::string _fname, std::deque<std::string> _args,
            FCALLTYPE _ftype = CLOSURE, types::Value _type = types::Float(),
            bool istimed = false)
      : MIRinstruction(_lv, _type),
        fname(std::move(_fname)),
        args(std::move(_args)),
        ftype(_ftype),
        istimed(istimed){};
  std::string toString() override;
  void closureConvert(std::vector<TypedVal>& fvlist,
                      std::shared_ptr<ClosureConverter> cc,
                      std::shared_ptr<MIRblock> mir,
                      std::list<Instructions>::iterator it) override;
};
class MakeClosureInst : public MIRinstruction {
 public:
  std::string fname;
  std::vector<TypedVal> captures;
  types::Value capturetype;
  std::string toString() override;
  MakeClosureInst(std::string _lv, std::string _fname,
                  std::vector<TypedVal> _captures, types::Value _captype)
      : MIRinstruction(_lv, types::Ref()),
        fname(std::move(_fname)),
        captures(std::move(_captures)),
        capturetype(std::move(_captype)) {}
  void closureConvert(std::vector<TypedVal>& fvlist,
                      std::shared_ptr<ClosureConverter> cc,
                      std::shared_ptr<MIRblock> mir,
                      std::list<Instructions>::iterator it) override;
};
class ArrayInst : public MIRinstruction {
  std::string name;
  std::deque<std::string> args;

 public:
  ArrayInst(std::string _lv, std::deque<std::string> _args)
      : MIRinstruction(_lv, types::Array(types::Float())),
        args(std::move(_args)) {}

  std::string toString() override;
  void closureConvert(std::vector<TypedVal>& fvlist,
                      std::shared_ptr<ClosureConverter> cc,
                      std::shared_ptr<MIRblock> mir,
                      std::list<Instructions>::iterator it) override;
};
class ArrayAccessInst : public MIRinstruction {
  std::string name;
  std::string index;

 public:
  ArrayAccessInst(std::string _lv, std::string _name, std::string _index)
      : MIRinstruction(_lv, types::Float()),
        name(std::move(_name)),
        index(std::move(_index)) {}
  std::string toString() override;
  void closureConvert(std::vector<TypedVal>& fvlist,
                      std::shared_ptr<ClosureConverter> cc,
                      std::shared_ptr<MIRblock> mir,
                      std::list<Instructions>::iterator it) override;
};
class IfInst : public MIRinstruction {
 public:
  std::string cond;
  std::shared_ptr<MIRblock> thenblock;
  std::shared_ptr<MIRblock> elseblock;
  IfInst(std::string name, std::string _cond)
      : MIRinstruction(name, types::Void()), cond(std::move(_cond)) {
    thenblock = std::make_shared<MIRblock>(name + "$then");
    elseblock = std::make_shared<MIRblock>(name + "$else");
  }
  std::string toString() override;
  void closureConvert(std::vector<TypedVal>& fvlist,
                      std::shared_ptr<ClosureConverter> cc,
                      std::shared_ptr<MIRblock> mir,
                      std::list<Instructions>::iterator it) override;
};
class ReturnInst : public MIRinstruction {
 public:
  std::string val;
  ReturnInst(std::string name, std::string _val,
             types::Value _type = types::Float())
      : MIRinstruction(name, _type), val(std::move(_val)) {}
  std::string toString() override;
  void closureConvert(std::vector<TypedVal>& fvlist,
                      std::shared_ptr<ClosureConverter> cc,
                      std::shared_ptr<MIRblock> mir,
                      std::list<Instructions>::iterator it) override;
};

}  // namespace mimium