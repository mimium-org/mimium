
#include "ast.hpp"




std::ostream& NumberAST::to_string(std::ostream& ss){
    //type matching
    ss << val;
    if(istimeset()){
            ss << "@" << std::to_string(get_time());
        }
    return ss;
    
}



std::ostream& SymbolAST::to_string(std::ostream& ss){
    ss <<  val;
    if(istimeset()){
        ss << "@" << std::to_string(get_time());
    }
    return ss;
}

OpAST::OpAST(std::string Op,AST_Ptr LHS, AST_Ptr RHS):op(Op),lhs(std::move(LHS)),rhs(std::move(RHS)){
        id=OP;
        op_id = optable[Op];
}

std::ostream& OpAST::to_string(std::ostream& ss){
        ss << "("<< op <<" ";
        lhs->to_string(ss);
        ss<< " ";
        rhs->to_string(ss);
        ss<<")";
        if(istimeset()){
            ss<< "@" << std::to_string(get_time());
        }
        return ss;
    }


std::ostream& ListAST::to_string(std::ostream& ss){
        ss << "(";
        for(auto &elem :asts){
            elem->to_string(ss);
            ss<< " ";
        }
        ss << ")";
        if(istimeset()){
            ss << "@" << std::to_string(get_time());
        }
        return ss;
    }

std::ostream& ArgumentsAST::to_string(std::ostream& ss){
        ss << "(";
        for(auto &elem :args){
            elem->to_string(ss);
            ss<< " ";
        }
        ss << ")";
        if(istimeset()){
            ss << "@" << std::to_string(get_time());
        }
        return ss;
    }

std::ostream& ReturnAST::to_string(std::ostream& ss){
        ss << "( return ";
            expr->to_string(ss);
        ss << ")";
        if(istimeset()){
            ss << "@" << std::to_string(get_time());
        }
        return ss;
    }


std::ostream& LambdaAST::to_string(std::ostream& ss){
        ss << "(lambda (";
        args->to_string(ss);
        ss <<")";
        body->to_string(ss) ;
        ss << ")";
        if(istimeset()){
            ss << "@" << std::to_string(get_time());
        }
        return ss;
    }


std::ostream& AssignAST::to_string(std::ostream& ss){
        ss << "("<< "assign" <<" ";
        symbol->to_string(ss);
        ss << " ";
        expr->to_string(ss);
        ss <<")";
        if(istimeset()){
            ss << "@" << std::to_string(get_time());
        }
        return ss;
}

std::ostream& FcallAST::to_string(std::ostream& ss){
        ss << "(";
        fname->to_string(ss);
        ss << " ";
        args->to_string(ss);
        ss <<")";
        if(istimeset()){
            ss << "@" << std::to_string(get_time());
        }
        return ss;
}
