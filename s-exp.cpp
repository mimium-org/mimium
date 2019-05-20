#include "s-exp.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <memory>

ListExpr::ListExpr(std::vector<std::string> strs){
    for(auto &str : strs){
        std::shared_ptr<S_Expr> s = std::make_shared<LiteralExpr>(str);
        exprs.push_back(s);
    }
}

void ListExpr::add_str(std::string str){
        std::shared_ptr<S_Expr> s = std::make_shared<LiteralExpr>(str);
        exprs.push_back(s);
};
void ListExpr::add_str(char ch){
        std::shared_ptr<S_Expr> s = std::make_shared<LiteralExpr>(ch);
        exprs.push_back(s);
};
void ListExpr::add_ptr(std::shared_ptr<S_Expr> ptr){
        exprs.push_back(ptr);
};


std::string ListExpr::to_string(){
        std::stringstream st;
        st << '(';
        for(int i=0;i<exprs.size();i++){
            st<< exprs[i]->to_string();
            if(i<exprs.size()-1){
            st<<',';
            }
        }
        st << ')';
        return st.str();
}