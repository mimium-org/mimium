#pragma once
#include <memory>
#include <string>
#include <vector>


enum AstID{
    BaseID,
    VariableID,
    NumberID,
    CallExprID,
    BinaryExprID,
    AssignID
};

class BaseAST{
    AstID ID;
    public:
    BaseAST(AstID id):ID(id){}
    virtual ~BaseAST(){}
    AstID getValueID()const{return ID;}
};

template<typename T>
class SingleAST:public BaseAST{
    T Val;
    public:
    SingleAST(const T &val):BaseAST(VariableID),Val(val){}
    ~SingleAST(){}

    T getVal(){return Val;}
};


class VariableAST :  public SingleAST<std::string>{
    public:
    VariableAST(const std::string & val):SingleAST<std::string>(val){}
    static inline bool classof(VariableAST const* ast){return true;}
    static inline bool classof(BaseAST const* baseast){
        return baseast->getValueID()==VariableID;
    }
};
class NumberAST : public BaseAST{
    int Val;
    public:
    NumberAST(int & val):BaseAST(NumberID),Val(val){}
    static inline bool classof(NumberAST const* ast){return true;}
    static inline bool classof(BaseAST const* baseast){
        return baseast->getValueID()==NumberID;
    }
    int getVal(){return Val;}

};

class FcallAST: public BaseAST{
    std::string Fname;
    std::vector<std::shared_ptr<BaseAST>> Args;

    public:
    FcallAST(const std::string &fname,std::vector<std::shared_ptr<BaseAST>> &args)
    : BaseAST(CallExprID),Fname(fname),Args(args){}
    ~FcallAST(){}

    static inline bool classof(FcallAST const* ast){return true;}
    static inline bool classof(BaseAST const* baseast){
        return baseast->getValueID()==CallExprID;
        }
    std::shared_ptr<BaseAST> getArgs(int i);
};

class BinaryExprAST: public BaseAST{
    std::string Op;
    BaseAST *LHS, *RHS;
    public:
    BinaryExprAST(std::string op,BaseAST *lhs,BaseAST *rhs)
    : BaseAST(BinaryExprID),Op(op),LHS(lhs),RHS(rhs){}
    static inline bool classof(BinaryExprAST const* ast){return true;}
    static inline bool classof(BaseAST const* baseast){
        return baseast->getValueID()==BinaryExprID;
        }
    std::string getOp(){return Op;}
    BaseAST* getLHS(){return LHS;}
    BaseAST*  getRHS(){return RHS;}

};

class AssignAST: public BaseAST{
    std::string Name;
    BaseAST& Assignee;
    public:
    AssignAST(std::string name,BaseAST& assignee):BaseAST(AssignID),Name(name),Assignee(assignee){}
    static inline bool classof(AssignAST const* ast){return true;}
    static inline bool classof(BaseAST const* baseast){
        return baseast->getValueID()==AssignID;
    }
    std::string getName(){return Name;}
    BaseAST& getAssignee(){return Assignee;}
};
