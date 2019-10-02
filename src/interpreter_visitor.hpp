#pragma once
#include <cmath>
#include "ast.hpp"
#include "helper_functions.hpp"
#include "interpreter.hpp"

namespace mimium{
    class InterpreterVisitor: public ASTVisitor, public std::enable_shared_from_this<InterpreterVisitor>{
        public:
        explicit InterpreterVisitor(Interpreter* _itp):itp(_itp){
        
        }
        template <class ASTTYPE>
        mValue jumptoaccept(AST* ast){
            return dynamic_cast<ASTTYPE>(ast)->template accept<mValue,InterpreterVisitor>(this);
        }
        mValue visit(AST* ast){
            switch(ast->getid()){
                case LIST:
                    return jumptoaccept<ListAST*>(ast);
                case ASSIGN:
                    return jumptoaccept<AssignAST*>(ast);
                case NUMBER:
                    return jumptoaccept<NumberAST*>(ast);
                // case SYMBOL:
                //     return jumptoaccept<SymbolAST*>(ast);
                case OP:
                    return jumptoaccept<OpAST*>(ast);
                break;
                default:
                    return 0.0;
            }
        }
        mValue visit(ListAST* ast){
            mValue res;
            for(auto& line :  ast->getlist()){
                res = line->accept<mValue>(this);
            }
            return res;
        }
        mValue visit(NumberAST* ast){
             return ast->getVal();
        };
        std::string visit(SymbolAST* ast){
             return ast->getVal();
        };
        mValue visit(OpAST* ast){
            double lv = std::get<double> ( ast->lhs->accept<mValue>(this) );
            double rv = std::get<double> ( ast->rhs->accept<mValue>(this) );
            switch (ast->getOpId()) {
                case ADD:
                return lv + rv;
                break;
                case SUB:
                return lv - rv;
                break;
                case MUL:
                return lv * rv;
                break;
                case DIV:
                return lv / rv;
                break;
                case EXP:
                return std::pow(lv, rv);
                break;
                case MOD:
                return std::fmod(lv, rv);
                break;
                case AND:
                case BITAND:
                return (double)((bool)lv & (bool)rv);
                break;
                case OR:
                case BITOR:
                return (double)((bool)lv | (bool)rv);
                break;
                case LT:
                return (double)lv < rv;
                break;
                case GT:
                return (double)lv > rv;
                break;
                case LE:
                return (double)lv <= rv;
                break;
                case GE:
                return (double)lv >= rv;
                break;
                case LSHIFT:
                return (double)((int)lv << (int)rv);
                break;
                case RSHIFT:
                return (double)((int)lv >> (int)rv);
                break;
                default:
                throw std::runtime_error("invalid binary operator");
                return 0.0;
            }

        }
        mValue visit(AssignAST* ast){
            return visit_assign(ast);
        }

        private:
        mValue visit_assign(AssignAST* ast);
        std::shared_ptr<Interpreter> itp;
    };
}