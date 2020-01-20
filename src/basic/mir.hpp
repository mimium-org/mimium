#pragma once
#include <list>
#include <utility>

#include "basic/ast.hpp"
#include "basic/type.hpp"

namespace mimium {

enum FCALLTYPE { DIRECT, CLOSURE, EXTERNAL };
static std::map<FCALLTYPE, std::string> fcalltype_str = {
    {DIRECT, ""}, {CLOSURE, "cls"}, {EXTERNAL, "ext"}};

class MIRblock;
struct MIRinstruction {  // base class for MIR instruction
  std::string lv_name;
  types::Value type;
  std::shared_ptr<MIRblock> parent = nullptr;
  MIRinstruction() = default;
  virtual ~MIRinstruction() = default;
  MIRinstruction(std::string lv_name, types::Value type = types::None())
      : lv_name(std::move(lv_name)), type(std::move(type)) {}
  virtual void setParent(std::shared_ptr<MIRblock> block) { parent = block; }
  virtual std::string toString() = 0;
  virtual bool isFunction() { return false; }
};

struct NumberInst : public MIRinstruction {
 public:
  NumberInst(std::string _lv, double _val)
      : MIRinstruction(_lv, types::Float()), val(std::move(_val)) {}
  double val;
  std::string toString() override;
};
struct AllocaInst : public MIRinstruction {
  AllocaInst(std::string lv, types::Value type = types::Float())
      : MIRinstruction(lv, type) {}
  std::string toString() override;
};
struct RefInst : public MIRinstruction {
  std::string val;
  RefInst(std::string _lv, std::string _val,
          types::Value _type = types::Float())
      : MIRinstruction(_lv, _type), val(std::move(_val)) {}
  std::string toString() override;
};
struct AssignInst : public MIRinstruction {
  std::string val;
  AssignInst(std::string _lv, std::string _val,
             types::Value _type = types::Float())
      : MIRinstruction(_lv, _type), val(std::move(_val)) {}
  std::string toString() override;
};

struct TimeInst : public MIRinstruction {
  std::string time;
  std::string val;
  TimeInst(std::string _lv, std::string _val, std::string _time,
           types::Value _type)
      : MIRinstruction(_lv, _type),
        time(std::move(_time)),
        val(std::move(_val)) {}
  std::string toString() override;
};
struct OpInst : public MIRinstruction {
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
};

struct FunInst : public MIRinstruction{
  std::deque<std::string> args;
  std::shared_ptr<MIRblock> body;
  std::vector<std::string> freevariables;  // introduced in closure conversion;
  bool isrecursive;
  explicit FunInst(std::string name, std::deque<std::string> newargs,
                   types::Value _type = types::Void(),
                   bool isrecursive = false);
  std::string toString() override;
  bool isFunction() override { return true; }
};
struct FcallInst : public MIRinstruction {
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
};
struct MakeClosureInst : public MIRinstruction {
  std::string fname;
  std::vector<std::string> captures;
  types::Value capturetype;
  std::string toString() override;
  MakeClosureInst(std::string _lv, std::string _fname,
                  std::vector<std::string> _captures, types::Value _captype)
      : MIRinstruction(_lv, types::Ref()),
        fname(std::move(_fname)),
        captures(std::move(_captures)),
        capturetype(std::move(_captype)) {}
};
struct ArrayInst : public MIRinstruction {
  std::string name;
  std::deque<std::string> args;
  ArrayInst(std::string _lv, std::deque<std::string> _args)
      : MIRinstruction(_lv, types::Array(types::Float())),
        args(std::move(_args)) {}

  std::string toString() override;
};
struct ArrayAccessInst : public MIRinstruction {
  std::string name;
  std::string index;
  ArrayAccessInst(std::string _lv, std::string _name, std::string _index)
      : MIRinstruction(_lv, types::Float()),
        name(std::move(_name)),
        index(std::move(_index)) {}
  std::string toString() override;
};
struct IfInst : public MIRinstruction {
  std::string cond;
  std::shared_ptr<MIRblock> thenblock;
  std::shared_ptr<MIRblock> elseblock;
  IfInst(std::string name, std::string _cond)
      : MIRinstruction(name, types::Void()), cond(std::move(_cond)) {
    thenblock = std::make_shared<MIRblock>(name + "$then");
    elseblock = std::make_shared<MIRblock>(name + "$else");
  }
  std::string toString() override;
};
struct ReturnInst : public MIRinstruction {
  std::string val;
  ReturnInst(std::string name, std::string _val,
             types::Value _type = types::Float())
      : MIRinstruction(name, _type), val(std::move(_val)) {}
  std::string toString() override;
};
using Instructions =
    std::variant<std::shared_ptr<NumberInst>, std::shared_ptr<AllocaInst>,
                 std::shared_ptr<RefInst>, std::shared_ptr<AssignInst>,
                 std::shared_ptr<TimeInst>, std::shared_ptr<OpInst>,
                 std::shared_ptr<FunInst>, std::shared_ptr<FcallInst>,
                 std::shared_ptr<MakeClosureInst>, std::shared_ptr<ArrayInst>,
                 std::shared_ptr<ArrayAccessInst>, std::shared_ptr<IfInst>,
                 std::shared_ptr<ReturnInst> >;

class MIRblock : public std::enable_shared_from_this<MIRblock> {
 public:
  explicit MIRblock(std::string _label)
      : label(std::move(_label)), prev(nullptr), next(nullptr) {
    indent_level = 0;
  };
  auto begin() { return instructions.begin(); }
  auto end() { return instructions.end(); }
  void addInst(Instructions& inst) {
    instructions.push_back(inst);
    std::visit(
        [&](auto& i) -> void {
          std::static_pointer_cast<MIRinstruction>(i)->setParent(
              shared_from_this());
        },
        inst);
  }
  void changeIndent(int level) { indent_level += level; }
  std::string label;
  std::list<Instructions> instructions;  // sequence of instructions
  std::shared_ptr<MIRblock> prev;
  std::shared_ptr<MIRblock> next;
  std::string toString();
  int indent_level;  // shared between instances
};

}  // namespace mimium