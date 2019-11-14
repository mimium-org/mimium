#pragma once

#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>  //pair
#include <variant>
#include <vector>

#include "type.hpp"

enum AST_ID {
  BASE,
  NUMBER,
  SYMBOL,
  ARG,
  ARGS,
  ARRAY,
  ARRAYACCESS,
  FCALL,
  DECLARATION,
  ASSIGN,
  FDEF,
  RETURN,
  ARRAYINIT,
  LAMBDA,
  OP,
  IF,
  FOR,
  LIST,
  TIME

};

class AST;  // forward
class ListAST;
class OpAST;
class NumberAST;
class SymbolAST;
class LvarAST;
class RvarAST;
class AssignAST;
class LambdaAST;
class AbstractListAST;
class ArgumentsAST;
class ArrayAST;
class ArrayAccessAST;
class TimeAST;
class ReturnAST;
class DeclarationAST;
class ForAST;
class IfAST;
class FcallAST;
class StructAST;  // currently internally used for closure conversion;
class StructAccessAST;

namespace mimium {
struct Closure;  // forward
};
using mClosure_ptr = std::shared_ptr<mimium::Closure>;

using AST_Ptr = std::shared_ptr<AST>;
using mValue = std::variant<double, std::shared_ptr<AST>, mClosure_ptr,
                            std::vector<double>, std::string>;

class ASTVisitor {
 public:
  virtual ~ASTVisitor() = default;
  virtual void visit(ListAST& ast) = 0;
  virtual void visit(OpAST& ast) = 0;
  virtual void visit(NumberAST& ast) = 0;
  virtual void visit(LvarAST& ast) = 0;
  virtual void visit(RvarAST& ast) = 0;
  virtual void visit(AssignAST& ast) = 0;
  virtual void visit(ArrayAST& ast) = 0;
  virtual void visit(ArgumentsAST& ast) = 0;
  virtual void visit(ArrayAccessAST& ast) = 0;
  virtual void visit(FcallAST& ast) = 0;
  virtual void visit(LambdaAST& ast) = 0;
  virtual void visit(IfAST& ast) = 0;
  virtual void visit(ReturnAST& ast) = 0;
  virtual void visit(ForAST& ast) = 0;
  virtual void visit(DeclarationAST& ast) = 0;
  virtual void visit(TimeAST& ast) = 0;
  virtual void visit(StructAST& ast) = 0;
  virtual void visit(StructAccessAST& ast) = 0;
  virtual mValue findVariable(std::string str) = 0;
  mValue stack_pop() {  // helper
    auto res = res_stack.top();
    res_stack.pop();
    return res;
  }

 protected:
  std::stack<mValue> res_stack;
};

class AST {
 public:
  virtual ~AST() = default;
  virtual std::string toString() = 0;
  virtual std::string toJson() = 0;
  virtual void accept(ASTVisitor& visitor) = 0;
  virtual void addAST(AST_Ptr ast){};  // for list/argument ast

  AST_ID getid() { return id; }

 protected:
  AST_ID id = BASE;
};

enum OP_ID {
  ADD,
  SUB,
  MUL,
  DIV,
  EXP,
  MOD,
  AND,
  OR,
  BITAND,
  BITOR,
  LE,
  GE,
  LT,
  GT,
  LSHIFT,
  RSHIFT
};
static std::map<std::string, OP_ID> optable = {
    {"+", ADD},     {"-", SUB},    {"*", MUL},     {"/", DIV},
    {"^", EXP},     {"%", MOD},    {"&", AND},     {"|", OR},
    {"&&", BITAND}, {"||", BITOR}, {"<", LT},      {">", GT},
    {"<=", LE},     {">=", GE},    {"<<", LSHIFT}, {">>", RSHIFT}};

class OpAST : public AST {
 public:
  std::string op;
  OP_ID op_id;
  AST_Ptr lhs, rhs;

  OpAST(std::string Op, AST_Ptr LHS, AST_Ptr RHS);
  // OpAST(OP_ID Op_id, AST_Ptr LHS, AST_Ptr RHS)
  //     : op_id(Op_id), lhs(std::move(LHS)), rhs(std::move(RHS)) {
  //   id = OP;
  
  // }
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  std::string toString() override;
  std::string toJson() override;
  OP_ID getOpId() { return op_id; };
  std::string getOpStr(){ return op;};
};

class ListAST : public AST {
 public:
  std::deque<AST_Ptr> asts;
  ListAST() {  // empty constructor
    id = LIST;
  }

  explicit ListAST(AST_Ptr _asts) {
    asts.push_back(std::move(_asts));  // push_front
    id = LIST;
  }
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  void addAST(AST_Ptr ast) override { asts.push_front(std::move(ast)); }
  void appendAST(AST_Ptr ast) {  // for knorm
    asts.push_back(std::move(ast));
  }
  inline auto& getElements() { return asts; };
  std::string toString() override;
  std::string toJson() override;
};
class NumberAST : public AST {
 public:
  double val;
  explicit NumberAST(double input) {
    val = input;
    id = NUMBER;
  }
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  double getVal() { return val; };
  std::string toString() override;
  std::string toJson() override;
};

class SymbolAST : public AST {
 public:
  std::string val;
  explicit SymbolAST(std::string input) : val(std::move(input)) { id = SYMBOL; }
  // void accept(ASTVisitor& visitor) override{
  //     visitor.visit(*this);
  // };
  std::string& getVal() { return val; };
  std::string toString() override;
  std::string toJson() override;
};
class LvarAST : public SymbolAST {
 public:
  mimium::types::Value type;
  explicit LvarAST(std::string input) : SymbolAST(input){
    type = mimium::types::Float();//default type is Float
  };
  explicit LvarAST(std::string input, mimium::types::Value _type):SymbolAST(input){
    type = _type;
  }
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};
class RvarAST : public SymbolAST {
 public:
  explicit RvarAST(std::string input) : SymbolAST(input){};
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

class AbstractListAST : public AST {
 public:
  std::deque<AST_Ptr> elements;
  AbstractListAST() {}  // do nothing
  explicit AbstractListAST(AST_Ptr arg) { elements.push_front(std::move(arg)); }
  void addAST(AST_Ptr arg) override { elements.push_front(std::move(arg)); };
  void appendAST(AST_Ptr arg) { elements.push_back(std::move(arg)); };
  auto& getElements() { return elements; }
  std::string toString() override;
  std::string toJson() override;
};

class ArgumentsAST : public AbstractListAST {
 public:
  ArgumentsAST() {}  // do nothing
  explicit ArgumentsAST(AST_Ptr arg)
      : AbstractListAST(std::static_pointer_cast<LvarAST>(std::move(arg))) {
    id = ARGS;
  };
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
};
class ArrayAST : public AbstractListAST {
 public:
  ArrayAST() {}  // do nothing
  explicit ArrayAST(AST_Ptr arg) : AbstractListAST(std::move(arg)) {
    id = ARRAY;
  };
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
};
class ArrayAccessAST : public AST {
 public:
  AST_Ptr name;
  AST_Ptr index;
  ArrayAccessAST(AST_Ptr Array, AST_Ptr Index)
      : name(std::move(Array)), index(std::move(Index)) {
    id = ARRAYACCESS;
  }
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  auto getName() { return std::static_pointer_cast<RvarAST>(name); };
  AST_Ptr getIndex() { return index; };
  std::string toString() override;
  std::string toJson() override;
};
class LambdaAST : public AST {
 public:
  AST_Ptr args;
  AST_Ptr body;  // statements
  LambdaAST(AST_Ptr Args, AST_Ptr Body)
      : args(std::move(Args)), body(std::move(Body)) {
    id = LAMBDA;
  }
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  auto getArgs() { return std::static_pointer_cast<ArgumentsAST>(args); };
  AST_Ptr getBody() { return body; };
  std::string toString() override;
  std::string toJson() override;
};

class AssignAST : public AST {
 public:
  AST_Ptr symbol;
  AST_Ptr expr;
  AssignAST(AST_Ptr Symbol, AST_Ptr Expr)
      : symbol(std::move(Symbol)), expr(std::move(Expr)) {
    id = ASSIGN;
  }
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  auto getName() { return std::static_pointer_cast<LvarAST>(symbol); };
  AST_Ptr getBody() { return expr; };
  std::string toString() override;
  std::string toJson() override;
};

class FcallAST : public AST {
 public:
  AST_Ptr fname;
  AST_Ptr args;
  FcallAST(AST_Ptr Fname, AST_Ptr Args)
      : fname(std::move(Fname)), args(std::move(Args)) {
    id = FCALL;
  }
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  auto getArgs() { return std::static_pointer_cast<ArgumentsAST>(args); };
  auto getFname() { return std::static_pointer_cast<SymbolAST>(fname); };
  std::string toString() override;
  std::string toJson() override;
};
class DeclarationAST : public AST {
 public:
  AST_Ptr fname;
  AST_Ptr args;
  DeclarationAST(AST_Ptr Fname, AST_Ptr Args)
      : fname(std::move(Fname)), args(std::move(Args)) {
    id = DECLARATION;
  }
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  auto getArgs() { return std::static_pointer_cast<ArgumentsAST>(args); };
  auto getFname() { return std::static_pointer_cast<SymbolAST>(fname); };
  std::string toString() override;
  std::string toJson() override;
};

class ReturnAST : public AST {
 public:
  AST_Ptr expr;
  explicit ReturnAST(AST_Ptr Expr) : expr(std::move(Expr)) { id = RETURN; }
  auto getExpr() { return expr; }
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  std::string toString() override;
  std::string toJson() override;
};
class IfAST : public AST {
 public:
  AST_Ptr condition, thenstatement, elsestatement;
  IfAST(AST_Ptr Condition, AST_Ptr Thenstatement, AST_Ptr Elsestatement)
      : condition(std::move(Condition)),
        thenstatement(std::move(Thenstatement)),
        elsestatement(std::move(Elsestatement)) {
    id = IF;
  }
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  auto getCond() { return condition; }
  auto getThen() { return thenstatement; }
  auto getElse() { return elsestatement; }
  std::string toString() override;
  std::string toJson() override;
};

class ForAST : public AST {
 public:
  AST_Ptr var, iterator, expression;
  ForAST(AST_Ptr Var, AST_Ptr Iterator, AST_Ptr Expression)
      : var(std::move(Var)),
        iterator(std::move(Iterator)),
        expression(std::move(Expression)) {
    id = FOR;
  }
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  auto getVar() { return var; };
  auto getIterator() { return iterator; };
  auto getExpression() { return expression; };
  std::string toString() override;
  std::string toJson() override;
};

class TimeAST : public AST {
 public:
  AST_Ptr expr;
  AST_Ptr time;
  explicit TimeAST(AST_Ptr Expr, AST_Ptr Time)
      : expr(std::move(Expr)), time(std::move(Time)) {
    id = TIME;
  }
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  auto getTime() { return time; }
  auto getExpr() { return expr; }
  std::string toString() override;
  std::string toJson() override;
};

class StructAST : public AST {
 public:
  std::unordered_map<AST_Ptr, AST_Ptr> map;
  StructAST() {}
  explicit StructAST(AST_Ptr key, AST_Ptr val) {
    map.emplace(std::move(key), std::move(val));
  }
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  void addPair(AST_Ptr key, AST_Ptr val) {
    map.emplace(std::move(key), std::move(val));
  }
  std::string toString() override;
  std::string toJson() override;
};
class StructAccessAST : public AST {
 public:
  AST_Ptr key;
  AST_Ptr val;

  explicit StructAccessAST(AST_Ptr _key, AST_Ptr _val)
      : key(std::move(_key)), val(std::move(_val)) {}
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  auto getKey() { return key; };
  auto getVal() { return val; };
  std::string toString() override;
  std::string toJson() override;
};