#pragma once

#include <vector>
#include <string>
#include <iostream>


struct S_Expr {
    virtual std::string to_string(){}
};

struct ListExpr : public S_Expr {
    std::vector<S_Expr*> exprs;
     
    std::string to_string(){
        std::stringstream st;
        std::string res;
        st << '(';
        for(int i=0;i<exprs.size();i++){
            st<< exprs[i]->to_string();
            if(i<exprs.size()-1){
            st<<',';
            }
        }
        st << ')';
        st>>res;
        return res;
    }
};

struct LiteralExpr : public S_Expr {
    std::string value;
    LiteralExpr(std::string val){value=val;}
    LiteralExpr(char val){
        std::string str{val};
        value = str;
        }

    std::string to_string(){
        return value;
    }
};
