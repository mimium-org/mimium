#pragma once
#include <string>
#include <memory>
#include "ast.hpp"
#include "mimium_parser.hpp"
#include "scanner.hpp"

template<class T>
AST_Ptr createAST_Ptr(T inputs){
    return std::dynamic_pointer_cast<AST>(std::make_unique<T>(inputs));
}

namespace mmmpsr{

    class MimiumDriver{
        public:
        MimiumDriver(){
            std::vector<AST_Ptr> vec;
            mainast  = std::make_unique<ListAST>(std::move(vec));
        };
        virtual ~MimiumDriver();
        void parse(std::istream &is);
        void parsestring(std::string& str);
        void parsefile(char *filename);


        AST_Ptr add_number(int num);

        AST_Ptr add_op(std::string op,int lhs,int rhs);
        AST_Ptr add_op(std::string op,AST_Ptr lhs,AST_Ptr rhs);
        AST_Ptr set_time(AST_Ptr elem,int time);
        void add_line(AST_Ptr in);
        std::ostream& print(std::ostream &stream);

        private:
            std::unique_ptr<ListAST> mainast;    
            std::unique_ptr<mmmpsr::MimiumParser>  parser  = nullptr;
            std::unique_ptr<mmmpsr::MimiumScanner> scanner = nullptr;
            const std::string red   = "\033[1;31m";
            const std::string blue  = "\033[1;36m";
            const std::string norm  = "\033[0m";
    };
};