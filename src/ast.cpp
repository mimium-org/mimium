
#include "ast.hpp"

std::string NumberAST::to_string(){
        std::stringstream stream;
        stream <<  std::to_string(val);
        if(istimeset()){
            stream << "@" << std::to_string(get_time());
        }
        return stream.str();
    
}

std::string OpAST::to_string(){
        std::stringstream stream;
        stream << "("<< op <<" " <<lhs->to_string() << " " << rhs->to_string() <<")";
        if(istimeset()){
            stream << "@" << std::to_string(get_time());
        }
        return stream.str();
    }


std::string ListAST::to_string(){
        std::stringstream stream;
        stream << "(";
        for(auto &elem :asts){
            stream << elem->to_string() << " ";
        }
        stream << ")";
        if(istimeset()){
            stream << "@" << std::to_string(get_time());
        }
        return stream.str();
    }
