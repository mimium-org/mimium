#include <iostream>
#include <fstream>

#include <string>
#include <sstream>

#include "driver.hpp"

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 0
#endif

namespace mmmpsr{

MimiumDriver::~MimiumDriver()
{

}
void MimiumDriver::parse(std::istream &is){
   scanner.reset();
   scanner = std::make_unique<mmmpsr::MimiumScanner>( is );
   parser.reset();
   parser = std::make_unique<mmmpsr::MimiumParser>( *scanner,*this );
   parser->set_debug_level(DEBUG_LEVEL); //debug
   if( parser->parse() != 0 ){
      std::cerr << "Parse failed!!\n";
   }
}
void MimiumDriver::parsestring(std::string &str){
   std::stringbuf strBuf( str.c_str() );
   std::istream instream( &strBuf );
   parse(instream);
}

void MimiumDriver::parsefile(std::string filename){
   std::ifstream ifs;
   ifs.exceptions(std::fstream::failbit | std::fstream::badbit);

   try{
      ifs.open(filename);
      parse(ifs);
   }catch( std::ios_base::failure e){
        std::cerr << "File read error" << std::endl;
        if(ifs.fail()){
         std::cerr << "no such file:" << filename <<std::endl;
        }
   }
}

AST_Ptr MimiumDriver::add_number(double num){
   return std::make_unique<NumberAST>(std::move(num));
}

AST_Ptr MimiumDriver::add_symbol(std::string str){
   return std::make_unique<SymbolAST>(std::move(str));
}

AST_Ptr MimiumDriver::add_op(tokentype op,AST_Ptr lhs,AST_Ptr rhs){
   switch (op)
   {
   case MimiumParser::token::ADD:
      return std::make_unique<AddAST>(std::move(lhs),std::move(rhs));
      break;
   case MimiumParser::token::SUB:
      return std::make_unique<SubAST>(std::move(lhs),std::move(rhs));
      break;
   case MimiumParser::token::MUL:
      return std::make_unique<MulAST>(std::move(lhs),std::move(rhs));
      break;
   case MimiumParser::token::DIV:
      return std::make_unique<DivAST>(std::move(lhs),std::move(rhs));
      break;
   default:
      std::cerr<< "the operator is not implemented yet" << std::endl;
      return nullptr;
      break;
   }
}
AST_Ptr MimiumDriver::add_op(std::string op,double lhs,double rhs){
      tokentype tt = op_map[op];
   return add_op(tt,std::make_unique<NumberAST>(lhs),std::make_unique<NumberAST>(rhs));
}
AST_Ptr MimiumDriver::add_op(std::string op,AST_Ptr lhs,AST_Ptr rhs){
   tokentype tt = op_map[op];
   return add_op(tt,std::move(lhs),std::move(rhs));
}


AST_Ptr MimiumDriver::add_assign(AST_Ptr symbol,AST_Ptr expr){
   return std::make_unique<AssignAST>(std::move(symbol),std::move(expr));
}
AST_Ptr MimiumDriver::add_arguments(AST_Ptr arg){
   return std::make_unique<ArgumentsAST>(std::move(arg));
}

AST_Ptr MimiumDriver::add_lambda(AST_Ptr args,AST_Ptr body){
   return std::make_unique<LambdaAST>(std::move(args),std::move(body));
}

AST_Ptr MimiumDriver::add_fcall(AST_Ptr fname,AST_Ptr args){
   return std::make_unique<FcallAST>(std::move(fname),std::move(args));
};


AST_Ptr MimiumDriver::add_statements(AST_Ptr statements){
   return std::make_unique<ListAST>(std::move(statements));
};


AST_Ptr MimiumDriver::set_time(AST_Ptr elem,int time){
    elem->set_time(time);
    return elem;
}

// void MimiumDriver::add_line(AST_Ptr in){
//    mainast->addAST(std::move(in));
// }
void MimiumDriver::add_top(AST_Ptr top){
     mainast = std::move(top);
}

std::ostream&  MimiumDriver::print( std::ostream &stream )
{
  mainast->to_string(stream);
   return(stream);
}

} // namespace mmmpsr