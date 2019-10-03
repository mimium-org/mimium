#pragma once

#include <map>
#include <utility>  //pair

#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

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

namespace mimium {
const std::map<std::string, AST_ID> ast_id = {{"fcall", FCALL},
                                              {"define", ASSIGN},
                                              {"fdef", FDEF},
                                              {"array", ARRAYINIT},
                                              {"lambda", LAMBDA}};
// static AST_ID str_to_id(std::string str){
//     auto id = mimium::ast_id.find(str);
//             if(id!=std::end(mimium::ast_id)){
//                     return id->second;
//             }else{
//                     return NUMBER;
//             }
// };
};          // namespace mimium
class AST;  // forward
class ListAST;
class OpAST;
class NumberAST;
class SymbolAST;
class AssignAST;
class LambdaAST;
class AbstractListAST;
class ArrayAST;
class ArrayAccessAST;
class TimeAST;
class ReturnAST;
class DeclarationAST;
class ForAST;
class IfAST;
class FcallAST;

namespace mimium {
struct Closure;  // forward
};
using mClosure_ptr = std::shared_ptr<mimium::Closure>;

using AST_Ptr = std::shared_ptr<AST>;
using mValue = std::variant<double, std::shared_ptr<AST>, mClosure_ptr,
                            std::vector<double> >;

class ASTVisitor{
  public:
      virtual ~ASTVisitor() = default;
      virtual mValue visit(ListAST& ast)=0;
      virtual mValue visit(OpAST& ast)=0;
      virtual mValue visit(NumberAST& ast)=0;
      virtual mValue visit(SymbolAST& ast)=0;
      virtual mValue visit(AssignAST& ast)=0;
      virtual mValue visit(AbstractListAST& ast)=0;
      virtual mValue visit(ArrayAST& ast)=0;
      virtual mValue visit(ArrayAccessAST& ast)=0;
      virtual mValue visit(FcallAST& ast)=0;
      virtual mValue visit(LambdaAST& ast)=0;
      virtual mValue visit(IfAST& ast)=0;
      virtual mValue visit(ReturnAST& ast)=0;
      virtual mValue visit(ForAST& ast)=0;
      virtual mValue visit(DeclarationAST& ast)=0;
      virtual mValue visit(TimeAST& ast)=0;

};

class AST {
 public:
  virtual ~AST() = default;
  virtual std::string toString() = 0;
  virtual std::string toJson() = 0;
  virtual mValue accept(ASTVisitor &visitor)=0;

  virtual void addAST(AST_Ptr ast){};  // for list/argument ast

  AST_ID getid() { return id; }
  bool istimeset() { return (time >= 0); }

 protected:
  AST_Ptr LogError(const char* Str) {
    std::cerr << "Error: " << Str << std::endl;
    return nullptr;
  }
  AST_ID id = BASE;

 private:
  int time = -1;
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
  OpAST(OP_ID Op_id, AST_Ptr LHS, AST_Ptr RHS)
      : op_id(Op_id), lhs(std::move(LHS)), rhs(std::move(RHS)) {
    id = OP;
  }
  mValue accept(ASTVisitor& visitor) override{
      return visitor.visit(*this);
  };
  std::string toString() override;
  std::string toJson() override;
  OP_ID getOpId() { return op_id; };

 protected:
  // static int getop(std::string op){return mimium::op_map[op];}
};

class ListAST : public AST {
 public:
  std::vector<AST_Ptr> asts;
  ListAST() {  // empty constructor
    id = LIST;
  }
  mValue accept(ASTVisitor& visitor) override{
      return visitor.visit(*this);
  };
  explicit ListAST(AST_Ptr _asts) {
    asts.push_back(std::move(_asts));  // push_front
    id = LIST;
  }
  void addAST(AST_Ptr ast) override {
    asts.insert(asts.begin(), std::move(ast));
  }
  std::vector<AST_Ptr>& getlist() { return asts; };
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
  mValue accept(ASTVisitor& visitor) override{
      return visitor.visit(*this);
  };
  double getVal() { return val; };
  std::string toString() override;
  std::string toJson() override;
};

class SymbolAST : public AST {
 public:
  std::string val;
  explicit SymbolAST(std::string input) : val(std::move(input)) { id = SYMBOL; }
  mValue accept(ASTVisitor& visitor) override{
      return visitor.visit(*this);
  };
  std::string& getVal() { return val; };
  std::string toString() override;
  std::string toJson() override;
};

class AbstractListAST : public AST {
 public:
  std::vector<AST_Ptr> elements;

  explicit AbstractListAST(AST_Ptr arg) {
    elements.insert(elements.begin(), std::move(arg));
  }
  void addAST(AST_Ptr arg) override {
    elements.insert(elements.begin(), std::move(arg));
  };
    mValue accept(ASTVisitor& visitor) override{
      return visitor.visit(*this);
  };
  auto& getElements() { return elements; }
  std::string toString() override;
  std::string toJson() override;
};

class ArgumentsAST : public AbstractListAST {
 public:
  explicit ArgumentsAST(AST_Ptr arg) : AbstractListAST(std::move(arg)) {
    id = ARGS;
  };
  mValue accept(ASTVisitor& visitor) override{
      return visitor.visit(*this);
  };
};
class ArrayAST : public AbstractListAST {
 public:
  explicit ArrayAST(AST_Ptr arg) : AbstractListAST(std::move(arg)) {
    id = ARRAY;
  };
    mValue accept(ASTVisitor& visitor) override{
      return visitor.visit(*this);
  };
};
class ArrayAccessAST : public AST {
 public:
  AST_Ptr name;
  AST_Ptr index;
  ArrayAccessAST(AST_Ptr Array, AST_Ptr Index)
      : name(std::move(Array)), index(std::move(Index)) {
    id = ARRAYACCESS;
  }
    mValue accept(ASTVisitor& visitor) override{
      return visitor.visit(*this);
  };
  AST_Ptr getName() { return name; };
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
    mValue accept(ASTVisitor& visitor) override{
      return visitor.visit(*this);
  };
  auto getArgs() { return std::dynamic_pointer_cast<ArgumentsAST>(args); };
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
  mValue accept(ASTVisitor& visitor) override{
      return visitor.visit(*this);
  };
  auto getName() { return std::dynamic_pointer_cast<SymbolAST>(symbol); };
  AST_Ptr getBody() { return expr; };
  std::string toString() override;
  std::string toJson() override;
};

// class FdefAST :  public AST{
//     public:
//     AST_Ptr fname;
//     AST_Ptr arguments;
//     AST_Ptr statements;
//     AssignAST(AST_Ptr Fname,AST_Ptr Arguments,AST_Ptr Statements):
//     fname(std::move(Fname)),arguments(std::move(Arguments)),statements(std::move(Statements)){
//         id=FDEF;
//     }
//     auto getFname(){return std::dynamic_pointer_cast<SymbolAST>(fname);};
//     auto getArguments(){return arguments;};
//     auto getFbody(){return statements;};

//     std::ostream& to_string(std::ostream& ss) override;
// };

class FcallAST : public AST {
 public:
  AST_Ptr fname;
  AST_Ptr args;
  FcallAST(AST_Ptr Fname, AST_Ptr Args)
      : fname(std::move(Fname)), args(std::move(Args)) {
    id = FCALL;
  }
  mValue accept(ASTVisitor& visitor) override{
      return visitor.visit(*this);
  };
  auto getArgs() { return std::dynamic_pointer_cast<ArgumentsAST>(args); };
  auto getFname() { return std::dynamic_pointer_cast<SymbolAST>(fname); };
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
  mValue accept(ASTVisitor& visitor) override{
      return visitor.visit(*this);
  };
  auto getArgs() { return std::dynamic_pointer_cast<ArgumentsAST>(args); };
  auto getFname() { return std::dynamic_pointer_cast<SymbolAST>(fname); };
  std::string toString() override;
  std::string toJson() override;
};

class ReturnAST : public AST {
 public:
  AST_Ptr expr;
  explicit ReturnAST(AST_Ptr Expr) : expr(std::move(Expr)) { id = RETURN; }
  auto getExpr() { return expr; }
  mValue accept(ASTVisitor& visitor) override{
      return visitor.visit(*this);
  };
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
  mValue accept(ASTVisitor& visitor) override{
      return visitor.visit(*this);
  };
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
  mValue accept(ASTVisitor& visitor) override{
      return visitor.visit(*this);
  };
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
  TimeAST(AST_Ptr Expr, AST_Ptr Time)
      : expr(std::move(Expr)), time(std::move(Time)) {
    id = TIME;
  }
  mValue accept(ASTVisitor& visitor) override{
      return visitor.visit(*this);
  };
  auto getTime() { return time; }
  auto getExpr() { return expr; }
  std::string toString() override;
  std::string toJson() override;
};