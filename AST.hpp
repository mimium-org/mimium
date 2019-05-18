#pragma once
#include <memory>
#include <string>
#include <vector>


enum AstID{
    BaseID,
    VariableID,
    NumberID,
    CallExprID,
    BinaryExprID
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
    SingleAST(const T &val):BaseAST(VariableID),Name(val){}
    ~SingleAST(){}
    static inline bool classof(SingleAST const* ast){return true;}
    static inline bool classof(BaseAST const* baseast){
        return baseast->getValueID()==VariableID;
    }
    T getVal(){return val;}
};


class VariableAST : private SingleAST<std::string>{};
class NumberAST : private SingleAST<int>{};

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
    std::shared_ptr<BaseAST> LHS,RHS;
    public:
    BinaryExprAST(std::string op,std::shared_ptr<BaseAST> lhs,std::shared_ptr<BaseAST> rhs)
    : BaseAST(BinaryExprID),Op(op),LHS(lhs),RHS(rhs){}
    static inline bool classof(BinaryExprAST const* ast){return true;}
    static inline bool classof(BaseAST const* baseast){
        return baseast->getValueID()==BinaryExprID;
        }
    std::string getOp(){return Op;}
    std::shared_ptr<BaseAST> getLHS(){return LHS;}
    std::shared_ptr<BaseAST> getRHS(){return RHS;}

};

class AssignAST: public BaseAST{
    std::string Name;
};
