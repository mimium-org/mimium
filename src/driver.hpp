#pragma once
#include <string>
#include <memory>

#include "mimium_parser.hpp"
#include "op_map.hpp"
#include "scanner.hpp"

template<class T>
AST_Ptr createAST_Ptr(T inputs){
    return std::dynamic_pointer_cast<AST>(std::make_unique<T>(inputs));
}
using tokentype = mmmpsr::MimiumParser::token::yytokentype;

namespace mmmpsr{

    class MimiumDriver{
        public:
        MimiumDriver(){
            std::vector<AST_Ptr> vec;
        };
        virtual ~MimiumDriver();
        void parse(std::istream &is);
        void parsestring(std::string& str);
        void parsefile(std::string filename);


        AST_Ptr add_number(int num);
        AST_Ptr add_symbol(std::string str);

        AST_Ptr add_op(std::string op,int lhs,int rhs);
        AST_Ptr add_op(std::string op,AST_Ptr lhs,AST_Ptr rhs);
        AST_Ptr add_op(tokentype op ,AST_Ptr lhs,AST_Ptr rhs);

        AST_Ptr add_arguments(AST_Ptr arg);
        AST_Ptr add_lambda(AST_Ptr args,AST_Ptr body);

        AST_Ptr add_assign(AST_Ptr symbol,AST_Ptr expr);
        AST_Ptr add_statements(AST_Ptr statements);

        AST_Ptr set_time(AST_Ptr elem,int time);
        // void add_line(AST_Ptr in); // final action
        void add_top(AST_Ptr top); // final action

        std::ostream& print(std::ostream &stream);

        private:
            std::unique_ptr<AST> mainast;    
            std::unique_ptr<AST> temporaryast;    

            std::unique_ptr<mmmpsr::MimiumParser>  parser  = nullptr;
            std::unique_ptr<mmmpsr::MimiumScanner> scanner = nullptr;
            const std::string red   = "\033[1;31m";
            const std::string blue  = "\033[1;36m";
            const std::string norm  = "\033[0m";

    };
};