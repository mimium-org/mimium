#include <iostream>
#include <fstream>

#include <string>
#include <sstream>

#include "driver.hpp"

namespace mmmpsr{

MimiumDriver::~MimiumDriver()
{

}
void MimiumDriver::parse(std::istream &is){
   scanner.reset();
   scanner = std::make_unique<mmmpsr::MimiumScanner>( is );
   parser.reset();
   parser = std::make_unique<mmmpsr::MimiumParser>( *scanner,*this );
   if( parser->parse() != 0 ){
      std::cerr << "Parse failed!!\n";
   }
}
void MimiumDriver::parsestring(std::string &str){
   std::stringbuf strBuf( str.c_str() );
   std::istream instream( &strBuf );
   parse(instream);
}

void MimiumDriver::parsefile(std::string &filename){
   std::ifstream ifs(filename);
   parse(ifs);
}

AST_Ptr MimiumDriver::add_number(int num){
   return createAST_Ptr<NumberAST>(num);
}
AST_Ptr MimiumDriver::add_op(std::string op,int lhs,int rhs){
   return add_op(op,createAST_Ptr<NumberAST>(lhs),createAST_Ptr<NumberAST>(rhs));
}
AST_Ptr MimiumDriver::add_op(std::string op,AST_Ptr lhs,AST_Ptr rhs){
   return std::dynamic_pointer_cast<AST>(std::make_shared<OpAST>(op,lhs,rhs));
}

AST_Ptr MimiumDriver::set_time(AST_Ptr elem,int time){
    elem->set_time(time);
    return elem;
}

void MimiumDriver::add_line(AST_Ptr in){
   mainast.addAST(in);
}

std::ostream&  MimiumDriver::print( std::ostream &stream )
{
   stream << mainast.to_string();
   return(stream);
}

} // namespace mmmpsr