
#include "ast.hpp"

std::string OpAST::to_string(){
        std::stringstream stream;
        stream << "("<< op <<" " <<lhs->to_string() << " " << rhs->to_string() <<")";
        return stream.str();
    }


std::string ListAST::to_string(){
        std::stringstream stream;
        stream << "(";
        for(auto &elem :asts){
            stream << elem->to_string() << " ";
        }
        stream << ")";
        return stream.str();
    }
