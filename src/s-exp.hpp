#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <memory>

struct S_Expr {
    S_Expr(){}
    S_Expr(S_Expr &s){};
    virtual ~S_Expr(){};
    virtual std::string to_string()=0;
    virtual void add_str(char ch){};
    virtual void add_str(std::string str){};
    virtual void add_ptr(std::shared_ptr<S_Expr> ptr){};
    virtual void add_vector(std::vector<std::shared_ptr<S_Expr>>& vec){};

};

struct ListExpr : public S_Expr {
    std::vector<std::shared_ptr<S_Expr>> exprs;
    ListExpr(){};
    ListExpr(std::vector<std::string> strs);
    ListExpr(std::vector<std::shared_ptr<S_Expr>> _exprs);
    ListExpr(ListExpr& self){};
    ~ListExpr(){};
    void add_str(std::string str);
    void add_str(char ch);
    void add_ptr(std::shared_ptr<S_Expr> ptr);
    void add_vector(std::vector<std::shared_ptr<S_Expr>>& vec);

    std::string to_string();
};

struct LiteralExpr : public S_Expr {
    std::string value;
    ~LiteralExpr(){};
    LiteralExpr(LiteralExpr& self){};
    LiteralExpr(std::string val):value(val){}
    LiteralExpr(char val){
        std::string str{val};
        value = str;
    }
    virtual void add_str(char ch){};
    virtual void add_str(std::string str){};
    virtual void add_ptr(std::shared_ptr<S_Expr> ptr){};
    virtual void add_vector(std::vector<std::shared_ptr<S_Expr>>& vec){};
    std::string to_string(){return value;}
};
