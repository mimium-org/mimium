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

void MimiumDriver::parsefile(char *filename){
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

AST_Ptr MimiumDriver::add_number(int num){
   return std::make_unique<NumberAST>(num);
}
AST_Ptr MimiumDriver::add_op(std::string op,int lhs,int rhs){
   return add_op(op,std::make_unique<NumberAST>(lhs),std::make_unique<NumberAST>(rhs));
}
AST_Ptr MimiumDriver::add_op(std::string op,AST_Ptr lhs,AST_Ptr rhs){
   return std::make_unique<OpAST>(op,std::move(lhs),std::move(rhs));
}

AST_Ptr MimiumDriver::set_time(AST_Ptr elem,int time){
    elem->set_time(time);
    return elem;
}

void MimiumDriver::add_line(AST_Ptr in){
   mainast->addAST(std::move(in));
}

std::ostream&  MimiumDriver::print( std::ostream &stream )
{
   stream << mainast->to_string();
   return(stream);
}

} // namespace mmmpsr