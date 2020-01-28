#pragma once
#include "basic/ast.hpp"
#include "basic/helper_functions.hpp"
#include "basic/type.hpp"
#include "compiler/ffi.hpp"
// type inference ... assumed to be visited after finished alpha-conversion(each
// variable has unique name regardless its scope)

namespace mimium {
struct TypevarReplaceVisitor {
  std::unordered_map<int, types::Value> tvmap;
  void replace();
  // default behaviour for primitive
  types::Value operator()(types::Float& i) { return i; }
  types::Value operator()(types::String& i) { return i; }
  types::Value operator()(types::None& i) { return i; }
  types::Value operator()(types::Void& i) { return i; }

  types::Value operator()(types::Ref& i) {
    return types::Ref(std::visit(*this, i.val));
  }
  types::Value operator()(types::Pointer& i) {
    return types::Pointer(std::visit(*this, i.val));
  }
  types::Value operator()(types::TypeVar& i) {
    types::Value res;
    if (tvmap.count(i.index) > 0) {
        res = std::visit(*this, tvmap[i.index]);     
    } else {
      // case of fail to infer
      Logger::debug_log("failed to infer type", Logger::WARNING);
      res = types::Float();
    }
    return res;
  }
  types::Value operator()(types::Function& i) {
    std::vector<types::Value> newarg;
    for (auto& a : i.arg_types) {
      newarg.emplace_back(std::visit(*this, a));
    }
    auto newret = std::visit(*this, i.ret_type);
    return types::Function(std::move(newret), std::move(newarg));
  }
  types::Value operator()(types::Closure& i) {
    auto newcap = std::visit(*this, i.captures);
    return types::Closure((*this)(i.fun), std::move(newcap));
  };
  types::Value operator()(types::Array& i) {
    auto newelem = std::visit(*this, i.elem_type);
    return types::Array(std::move(newelem), i.size);
  }
  types::Value operator()(types::Struct& i) {
    std::vector<types::Struct::Keytype> newarg;
    for (auto& a : i.arg_types) {
      types::Struct::Keytype v = {a.field, std::visit(*this, a.val)};
      newarg.emplace_back(std::move(v));
    }
    return types::Struct(std::move(newarg));
  }
  types::Value operator()(types::Tuple& i) {
    std::vector<types::Value> newarg;
    for (auto& a : i.arg_types) {
      newarg.emplace_back(std::visit(*this, a));
    }
    return types::Tuple(std::move(newarg));
  }
  types::Value operator()(types::Time& i) {
    return types::Time(std::visit(*this, i.val));
  }
  types::Value operator()(types::Alias& i) {
    return types::Alias(i.name, std::visit(*this, i.target));
  };
};
class TypeInferVisitor : public ASTVisitor {
  friend TypevarReplaceVisitor;

 public:
  TypeInferVisitor();
  ~TypeInferVisitor() override = default;
  void init();
  void visit(OpAST& ast) override;
  void visit(ListAST& ast) override;
  void visit(NumberAST& ast) override;
  void visit(LvarAST& ast) override;
  void visit(RvarAST& ast) override;
  void visit(SelfAST& ast) override;
  void visit(AssignAST& ast) override;
  void visit(ArgumentsAST& ast) override;
  void visit(FcallArgsAST& ast) override;
  void visit(ArrayAST& ast) override;
  void visit(ArrayAccessAST& ast) override;
  void visit(FcallAST& ast) override;
  void visit(LambdaAST& ast) override;
  void visit(IfAST& ast) override;
  void visit(ReturnAST& ast) override;
  void visit(ForAST& ast) override;
  void visit(DeclarationAST& ast) override;
  void visit(TimeAST& ast) override;
  void visit(StructAST& ast) override;
  void visit(StructAccessAST& ast) override;

  bool typeCheck(types::Value& lt, types::Value& rt);
  bool unify(types::Value& lt, types::Value& rt);

  bool unify(std::string lname, std::string rname);
  bool unify(std::string lname, types::Value& rt);
  bool unifyArg(types::Value& target, types::Value& realarg);

  TypeEnv& getEnv() { return typeenv; };
  types::Value getLastType() { return res_stack.top(); }
  types::Value stackPop() {
    auto res = res_stack.top();
    res_stack.pop();
    return res;
  }
  std::string tmpfname;

  TypeEnv& infer(AST_Ptr toplevel);
  void replaceTypeVars();

 private:
  std::stack<types::Value> res_stack;
  static bool checkArg(types::Value& fnarg, types::Value& givenarg);
  void unifyTypeVar(types::TypeVar& tv, types::Value& v);
  // hold value for infer type of "self"
  std::optional<types::Value> current_return_type;
  TypeEnv typeenv;
  TypevarReplaceVisitor tvreplacevisitor;
  // std::unordered_map<int, types::Value> typevar_to_actual_map;
  bool has_return;
};

}  // namespace mimium