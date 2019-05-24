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
ListExpr::ListExpr(std::vector<std::shared_ptr<S_Expr>> _exprs){
        for(auto &expr : _exprs){
        exprs.push_back(std::move(expr));
    }
};

void ListExpr::add_str(std::string str){
        std::shared_ptr<S_Expr> s = std::make_shared<LiteralExpr>(str);
        exprs.push_back(s);
};
void ListExpr::add_str(char ch){
        std::shared_ptr<S_Expr> s = std::make_shared<LiteralExpr>(ch);
        exprs.push_back(s);
};
void ListExpr::add_ptr(std::shared_ptr<S_Expr> ptr){
        exprs.push_back(std::move(ptr));
};

void ListExpr::add_vector(std::vector<std::shared_ptr<S_Expr>>& vec){
        exprs.insert(exprs.end(),vec.begin(),vec.end());
};

std::shared_ptr<S_Expr> ListExpr::get_head(){
        if(exprs.size()<1){
                std::cerr<<"List Empty" <<std::endl;
        }else{
                return exprs[0];
        }
}
std::shared_ptr<S_Expr> ListExpr::get_tail(){
        if(exprs.size()<1){
                std::cerr<<"List Empty" <<std::endl;
        }else if(exprs.size()==2){
                return exprs[1];
        }else{
                std::vector<std::shared_ptr<S_Expr>> vec;
                std::copy(exprs.begin()+1,exprs.end(),vec.begin());
                return std::make_shared<ListExpr>(vec);
        }
}

std::string ListExpr::to_string(){
        std::stringstream st;
        st << '(';
        for(int i=0;i<exprs.size();i++){
                if(exprs[i]!=nullptr){
                        st<< exprs[i]->to_string();
                }else{
                        st<<"null";
                }
            if(i<exprs.size()-1){
            st<<' ';
            }
        }
        st << ')';
        return st.str();
}