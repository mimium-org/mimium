#pragma once
#include <string>
#include <memory>
#include "ast_definitions.hpp"
#include "mimium_parser.hpp"
#include "scanner.hpp"

template<class T>
AST_Ptr createAST_Ptr(T inputs){
    return std::dynamic_pointer_cast<AST>(std::make_shared<T>(inputs));
}

namespace mmmpsr{

    class MimiumDriver{
        public:
        MimiumDriver() = default;
        virtual ~MimiumDriver();
        void parse(std::string& str);
        AST_Ptr add_number(int num);

        AST_Ptr add_op(std::string op,int lhs,int rhs);
        // AST_Ptr add_op(std::string op,AST_Ptr lhs,int rhs);
        // AST_Ptr add_op(std::string op,int lhs,AST_Ptr rhs);
        AST_Ptr add_op(std::string op,AST_Ptr lhs,AST_Ptr rhs);

        void add_line(AST_Ptr in);
        std::ostream& print(std::ostream &stream);

        private:
            ListAST mainast;    
            std::unique_ptr<mmmpsr::MimiumParser>  parser  = nullptr;
            std::unique_ptr<mmmpsr::MimiumScanner> scanner = nullptr;
            const std::string red   = "\033[1;31m";
            const std::string blue  = "\033[1;36m";
            const std::string norm  = "\033[0m";
    };
};