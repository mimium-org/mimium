#pragma once

#include "basic/helper_functions.hpp"
#include "basic/type.hpp"

namespace mimium {

enum AST_ID {
  BASE,
  NUMBER,
  LVAR,
  RVAR,
  ARG,
  ARGS,
  FCALLARGS,
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
class OpAST;
class NumberAST;
class SymbolAST;
class LvarAST;
class RvarAST;
class AssignAST;
class LambdaAST;
template <typename T, AST_ID IDTYPE>
class AbstractListAST;
using ArgumentsAST = AbstractListAST<std::shared_ptr<LvarAST>, ARGS>;
using FcallArgsAST = AbstractListAST<std::shared_ptr<AST>, FCALLARGS>;
using ListAST = AbstractListAST<std::shared_ptr<AST>, LIST>;

using ArrayAST = AbstractListAST<std::shared_ptr<AST>, ARRAY>;

class ArrayAccessAST;
class TimeAST;
class ReturnAST;
class DeclarationAST;
class ForAST;
class IfAST;
class FcallAST;
class StructAST;  // currently internally used for closure conversion;
class StructAccessAST;

using AST_Ptr = std::shared_ptr<AST>;

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
  virtual void visit(FcallArgsAST& ast) = 0;

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

 protected:
};

class AST {
 public:
  AST() : id(BASE){};
  explicit AST(AST_ID id) : id(id) {}
  virtual ~AST() = default;
  virtual std::string toString() = 0;
  virtual std::string toJson() = 0;
  virtual void accept(ASTVisitor& visitor) = 0;

  AST_ID getid() { return id; }

 protected:
  AST_ID id;
};

enum class OP_ID {
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
    {"+", OP_ID::ADD},    {"-", OP_ID::SUB}, {"*", OP_ID::MUL},
    {"/", OP_ID::DIV},    {"^", OP_ID::EXP}, {"%", OP_ID::MOD},
    {"&", OP_ID::AND},    {"|", OP_ID::OR},  {"&&", OP_ID::BITAND},
    {"||", OP_ID::BITOR}, {"<", OP_ID::LT},  {">", OP_ID::GT},
    {"<=", OP_ID::LE},    {">=", OP_ID::GE}, {"<<", OP_ID::LSHIFT},
    {">>", OP_ID::RSHIFT}};

class OpAST : public AST {
 public:
  std::string op;
  OP_ID op_id;
  AST_Ptr lhs, rhs;

  OpAST(std::string Op, AST_Ptr LHS, AST_Ptr RHS);
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  std::string toString() override;
  std::string toJson() override;
  OP_ID getOpId() { return op_id; };
  std::string getOpStr() { return op; };
};

class NumberAST : public AST {
 public:
  double val;
  explicit NumberAST(double input) : AST(NUMBER),val(input) {}
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  double getVal() { return val; };
  std::string toString() override;
  std::string toJson() override;
};

class SymbolAST : public AST {
 public:
  std::string val;
  explicit SymbolAST(std::string input) : val(std::move(input)) {}

  std::string& getVal() { return val; };
  std::string toString() override;
  std::string toJson() override;
};
class LvarAST : public SymbolAST {
 public:
  mimium::types::Value type;
  explicit LvarAST(std::string input) : SymbolAST(input) {
    type = mimium::types::None();  // default type is Float
    id = LVAR;
  };
  explicit LvarAST(std::string input, mimium::types::Value _type)
      : SymbolAST(input) {
    type = _type;
  }
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
  auto& getType() { return type; }
};
class RvarAST : public SymbolAST {
 public:
  explicit RvarAST(std::string input) : SymbolAST(input) { id = RVAR; };
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); }
};

template <typename T, AST_ID IDTYPE>
class AbstractListAST : public AST {
 protected:
  AST_ID id = IDTYPE;

 public:
  std::deque<T> elements;

  AbstractListAST() = default;  // do nothing
  explicit AbstractListAST(T arg) { elements.push_front(std::move(arg)); }
  void addAST(T arg) { elements.push_front(std::move(arg)); };
  void appendAST(T arg) { elements.push_back(std::move(arg)); };
  auto& getElements() { return elements; }
  std::string toString() override {
    return "(" + mimium::join(elements, " ") + ")";
  };
  std::string toJson() override {
    return "[" + mimium::join(elements, " , ") + "]";
  };
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
};

class ArrayAccessAST : public AST {
 public:
  std::shared_ptr<RvarAST> name;
  AST_Ptr index;
  ArrayAccessAST(std::shared_ptr<RvarAST> Array, AST_Ptr Index)
      : name(std::move(Array)), index(std::move(Index)) {
    id = ARRAYACCESS;
  }
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  auto getName() { return name; };
  auto getIndex() { return index; };
  std::string toString() override;
  std::string toJson() override;
};
class LambdaAST : public AST {
 public:
  std::shared_ptr<ArgumentsAST> args;
  AST_Ptr body;  // statements
  mimium::types::Value type;
  bool isrecursive = false;
  LambdaAST(std::shared_ptr<ArgumentsAST> Args, AST_Ptr Body,
            mimium::types::Value type = mimium::types::None())
      : args(std::move(Args)), body(std::move(Body)), type(std::move(type)) {
    id = LAMBDA;
  }

  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  auto getArgs() { return args; };
  AST_Ptr getBody() { return body; };
  std::string toString() override;
  std::string toJson() override;
};

class AssignAST : public AST {
 public:
  std::shared_ptr<LvarAST> symbol;
  AST_Ptr expr;
  AssignAST(std::shared_ptr<LvarAST> Symbol, AST_Ptr Expr)
      : symbol(std::move(Symbol)), expr(std::move(Expr)) {
    id = ASSIGN;
  }
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  auto getName() { return symbol; };
  AST_Ptr getBody() { return expr; };
  std::string toString() override;
  std::string toJson() override;
};

class FcallAST : public AST {
 public:
  std::shared_ptr<RvarAST> fname;
  std::shared_ptr<FcallArgsAST> args;
  FcallAST(std::shared_ptr<RvarAST> Fname, std::shared_ptr<FcallArgsAST> Args)
      : fname(std::move(Fname)), args(std::move(Args)) {
    id = FCALL;
  }
  void accept(ASTVisitor& visitor) override { visitor.visit(*this); };
  auto getArgs() { return args; }
  auto getFname() { return fname; }
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

}  // namespace mimium