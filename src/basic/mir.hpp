/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include <list>
#include <utility>

#include "basic/ast_new.hpp"
#include "basic/type.hpp"

namespace mimium {

enum FCALLTYPE { DIRECT, CLOSURE, EXTERNAL };
static std::map<FCALLTYPE, std::string> fcalltype_str = {
    {DIRECT, ""}, {CLOSURE, "cls"}, {EXTERNAL, "ext"}};

// struct uniquestr{
//   std::string str;
//   unsigned int count;
//   inline static unsigned int global_count=0;
//   uniquestr(std::string& s):str(s),count(global_count++){};
//   uniquestr(std::string&& s):str(std::move(s)),count(global_count++){};
//   uniquestr(char* s):str(s),count(global_count++){};
//   operator std::string(){return str+std::to_string(count);}
// };

class MIRblock;
struct MIRinstruction {  // base class for MIR instruction
  std::string lv_name;
  types::Value type;
  std::shared_ptr<MIRblock> parent = nullptr;
  MIRinstruction() = default;
  virtual ~MIRinstruction() = default;
  explicit MIRinstruction(std::string lv_name,
                          types::Value type = types::None())
      : lv_name(std::move(lv_name)), type(std::move(type)) {}
  virtual void setParent(std::shared_ptr<MIRblock> block) { parent = block; }
  virtual std::string toString() = 0;
  virtual bool isFunction() { return false; }
};

struct NumberInst : public MIRinstruction {
 public:
  NumberInst(const std::string& lv, double val)
      : MIRinstruction(lv, types::Float()), val(val) {}
  NumberInst():NumberInst("",0.0){}
  double val;
  std::string toString() override;
};
struct StringInst : public MIRinstruction {
 public:
  StringInst(const std::string& lv, std::string& val)
      : MIRinstruction(lv, types::String()), val(val) {}
  std::string val;
  std::string toString() override;
};
struct AllocaInst : public MIRinstruction {
  explicit AllocaInst(const std::string& lv, types::Value type = types::Float())
      : MIRinstruction(lv, type) {}
  std::string toString() override;
};
struct RefInst : public MIRinstruction {
  std::string val;
  RefInst(const std::string& lv, std::string val,
          types::Value type = types::Float())
      : MIRinstruction(lv, type), val(std::move(val)) {}
  std::string toString() override;
};
struct AssignInst : public MIRinstruction {
  std::string val;
  AssignInst(const std::string& lv, std::string val,
             types::Value type = types::Float())
      : MIRinstruction(lv, type), val(std::move(val)) {}
  std::string toString() override;
};

// struct TimeInst : public MIRinstruction {
//   std::string time;
//   std::string val;
//   TimeInst(const std::string& lv, std::string val, std::string time,
//            types::Value type)
//       : MIRinstruction(lv, type),
//         time(std::move(time)),
//         val(std::move(val)) {}
//   std::string toString() override;
// };
struct OpInst : public MIRinstruction {
 public:
  ast::OpId op;
  std::string lhs;
  std::string rhs;

  OpInst(const std::string& lv, ast::OpId op, std::string lhs,
         std::string rhs)
      : MIRinstruction(lv, types::Float()),
        op(op),
        lhs(std::move(lhs)),
        rhs(std::move(rhs)){};
  std::string toString() override;
};

struct FunInst : public MIRinstruction {
  std::deque<std::string> args;
  std::shared_ptr<MIRblock> body;
  std::vector<std::string> freevariables;  // introduced in closure conversion;
  // introduced after closure conversion;contains self & delay, and fcall which
  // has self&delay
  std::vector<std::string> memory_objects;

  bool ccflag = false;  // utility for closure conversion
  bool hasself;
  bool isrecursive;
  explicit FunInst(const std::string& name, std::deque<std::string> newargs,
                   types::Value type = types::Void(), bool isrecursive = false);
  std::string toString() override;
  bool isFunction() override { return true; }
};
struct FcallInst : public MIRinstruction {
  std::string fname;
  std::deque<std::string> args;
  std::optional<std::string> time;
  FCALLTYPE ftype;
  FcallInst(const std::string& lv, std::string fname,
            std::deque<std::string> args, FCALLTYPE ftype = CLOSURE,
            types::Value type = types::Float(),
            std::optional<std::string> time = std::nullopt)
      : MIRinstruction(lv, type),
        fname(std::move(fname)),
        args(std::move(args)),
        ftype(ftype),
        time(std::move(time)){};
  std::string toString() override;
};
struct MakeClosureInst : public MIRinstruction {
  std::string fname;
  std::vector<std::string> captures;
  types::Value capturetype;
  std::string toString() override;
  MakeClosureInst(const std::string& lv, std::string fname,
                  std::vector<std::string> captures, types::Value captype)
      : MIRinstruction(lv, types::Ref()),
        fname(std::move(fname)),
        captures(std::move(captures)),
        capturetype(std::move(captype)) {}
};
struct ArrayInst : public MIRinstruction {
  std::string name;
  int size;
  std::deque<std::string> args;
  ArrayInst(const std::string& lv, std::deque<std::string> args)
      : MIRinstruction(lv, types::Array(types::Float(), args.size())),
        args(std::move(args)),
        size(0) {}

  std::string toString() override;
};
struct ArrayAccessInst : public MIRinstruction {
  std::string name;
  std::string index;
  ArrayAccessInst(const std::string& lv, std::string name, std::string index)
      : MIRinstruction(lv, types::Float()),
        name(std::move(name)),
        index(std::move(index)) {}
  std::string toString() override;
};
struct IfInst : public MIRinstruction {
  std::string cond;
  std::shared_ptr<MIRblock> thenblock;
  std::shared_ptr<MIRblock> elseblock;
  IfInst(const std::string& name, std::string cond)
      : MIRinstruction(name, types::Void()), cond(std::move(cond)) {
    thenblock = std::make_shared<MIRblock>(name + "$then");
    elseblock = std::make_shared<MIRblock>(name + "$else");
  }
  std::string toString() override;
};
struct ReturnInst : public MIRinstruction {
  std::string val;
  ReturnInst(const std::string& name, std::string val,
             types::Value type = types::Float())
      : MIRinstruction(name, type), val(std::move(val)) {}
  std::string toString() override;
};
using Instructions =
    std::variant<NumberInst, StringInst, AllocaInst, RefInst, AssignInst,
                 OpInst, FunInst, FcallInst, MakeClosureInst, ArrayInst,
                 ArrayAccessInst, IfInst, ReturnInst>;


class MIRblock : public std::enable_shared_from_this<MIRblock> {
 public:
  MIRblock(MIRblock& origin) = default;
  explicit MIRblock(std::string label)
      : label(std::move(label)), prev(nullptr), next(nullptr) {
    indent_level = 0;
  };
  auto begin() { return instructions.begin(); }
  auto end() { return instructions.end(); }
  void addInst(Instructions& inst) {
    std::visit([&](auto& i) { i.setParent(shared_from_this()); }, inst);
    instructions.emplace_back(inst);
  }
  Instructions& addInstRef(Instructions&& inst) {
    std::visit([&](auto& i) { i.setParent(shared_from_this()); }, inst);
    return instructions.emplace_back(std::move(inst));
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