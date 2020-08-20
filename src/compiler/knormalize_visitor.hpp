/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/ast.hpp"
#include "basic/mir.hpp"
#include "compiler/ffi.hpp"
#include "compiler/type_infer_visitor.hpp"

namespace mimium {
class KNormalizeVisitor : public ASTVisitor {
 public:
  explicit KNormalizeVisitor(TypeInferVisitor& typeinfer);
  KNormalizeVisitor();
  ~KNormalizeVisitor() override = default;
  void init();

  void visit(OpAST& ast) override;
  void visit(ListAST& ast) override;
  void visit(NumberAST& ast) override;
  void visit(StringAST& ast) override;

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
  // void visit(TimeAST& ast) override;
  void visit(StructAST& ast) override;
  void visit(StructAccessAST& ast) override;

  std::shared_ptr<MIRblock> getResult();

 private:
  TypeInferVisitor typeinfer;  // to resolve anonymous function type;

  std::shared_ptr<MIRblock> rootblock;
  std::shared_ptr<MIRblock> currentblock;
  int var_counter;
  std::string makeNewName();
  std::string getVarName();
  bool isArgTime(FcallArgsAST& args);
  std::string tmpname;
  std::shared_ptr<ListAST> current_context;
  AST_Ptr insertAssign(AST_Ptr ast);
  void insertOverWrite(AST_Ptr body, const std::string& name);
  void insertAlloca(AST_Ptr body, const std::string& name);
  void insertRef(AST_Ptr body, const std::string& name);

  std::stack<std::string> res_stack_str;
  std::stack<types::Value> type_stack;
  std::vector<std::string> lvar_list;
  std::string stackPopStr() {
    auto ret = res_stack_str.top();
    res_stack_str.pop();
    return ret;
  }
};

using lvarid = std::pair<std::string, types::Value>;
class MirGenerator {
 public:
  explicit MirGenerator(TypeEnv& typeenv)
      : statementvisitor(*this), exprvisitor(*this), typeenv(typeenv),ctx(nullptr){
  }
  struct ExprKnormVisitor : public VisitorBase<lvarid&> {
    explicit ExprKnormVisitor(MirGenerator& parent) : mirgen(parent) {}
    lvarid operator()(newast::Op& ast);
    lvarid operator()(newast::Number& ast);
    lvarid operator()(newast::String& ast);
    lvarid operator()(newast::Rvar& ast);
    lvarid operator()(newast::Self& ast);
    lvarid operator()(newast::Lambda& ast);
    lvarid operator()(newast::Fcall& ast,
                      std::optional<std::string> when = std::nullopt);
    lvarid operator()(newast::Time& ast);
    lvarid operator()(newast::Struct& ast);
    lvarid operator()(newast::StructAccess& ast);
    lvarid operator()(newast::ArrayInit& ast);
    lvarid operator()(newast::ArrayAccess& ast);
    lvarid operator()(newast::Tuple& ast);

   private:
    MirGenerator& mirgen;
  };
  struct StatementKnormVisitor : public VisitorBase<lvarid&> {
    explicit StatementKnormVisitor(MirGenerator& parent) : mirgen(parent) {}
    lvarid operator()(newast::Assign& ast);
    lvarid operator()(newast::Return& ast);
    lvarid operator()(newast::For& ast);
    lvarid operator()(newast::If& ast);
    lvarid operator()(newast::ExprPtr& ast);
    // Instructions operator()(newast::Declaration& ast);

   private:
    MirGenerator& mirgen;
  };
  std::shared_ptr<MIRblock> generate(newast::Statements& topast);
  std::pair<lvarid, std::shared_ptr<MIRblock>> generateBlock(
      newast::Statements stmts, std::string label);
  bool isOverWrite(std::string const& name) {
    return std::find(lvarlist.begin(), lvarlist.end(), name) != lvarlist.end();
  }
  lvarid emplace(Instructions&& inst, types::Value type = types::Float()) {
    auto& newname =
        std::visit([](auto&& i) -> std::string& { return i.lv_name; },
                   ctx->addInstRef(std::move(inst)));
    typeenv.emplace(newname, type);
    return std::pair(newname, type);
  }
  template <typename FROM, typename TO, class LAMBDA>
  auto transformArgs(FROM&& from, TO&& to, LAMBDA&& op) -> decltype(to) {
    std::transform(from.begin(), from.end(), std::back_inserter(to), op);
    return std::forward<decltype(to)>(to);
  }
  static bool isExternalFun(std::string const& str) {
    return LLVMBuiltin::ftable.find(str) != LLVMBuiltin::ftable.end();
  }

 private:
  StatementKnormVisitor statementvisitor;
  ExprKnormVisitor exprvisitor;
  TypeEnv& typeenv;
  std::vector<std::string> lvarlist;
  std::optional<std::string> lvar_holder;
  std::shared_ptr<MIRblock> ctx = nullptr;
  std::stack<types::Value> selftype_stack;
  int64_t varcounter = 0;
  std::string makeNewName();
};

}  // namespace mimium