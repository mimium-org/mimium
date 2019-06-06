#include <iostream>
#include <string>
#include <sstream>

#include "driver.hpp"


mmmpsr::MimiumDriver::~MimiumDriver()
{

}

void mmmpsr::MimiumDriver::parse(std::string &str){
   std::stringbuf strBuf( str.c_str() );
   std::istream instream( &strBuf );
    scanner.reset();
   try
   {
      scanner = std::make_unique<mmmpsr::MimiumScanner>( instream );
   }
      catch( std::bad_alloc &ba )
   {
      std::cerr << "Failed to allocate scanner: (" <<
         ba.what() << "), exiting!!\n";
      exit( EXIT_FAILURE );
   }
    parser.reset();
   try
   {
      parser = std::make_unique<mmmpsr::MimiumParser>( *scanner,*this );
   }
      catch( std::bad_alloc &ba )
   {
      std::cerr << "Failed to allocate parser: (" << 
         ba.what() << "), exiting!!\n";
      exit( EXIT_FAILURE );
   }
      const int accept( 0 );
   if( parser->parse() != accept )
   {
      std::cerr << "Parse failed!!\n";
   }
}
AST_Ptr mmmpsr::MimiumDriver::add_number(int num){
   return createAST_Ptr<NumberAST>(num);
}
AST_Ptr mmmpsr::MimiumDriver::add_op(std::string op,int lhs,int rhs){
   return add_op(op,createAST_Ptr<NumberAST>(lhs),createAST_Ptr<NumberAST>(rhs));
}
AST_Ptr mmmpsr::MimiumDriver::add_op(std::string op,AST_Ptr lhs,AST_Ptr rhs){
   return std::dynamic_pointer_cast<AST>(std::make_shared<OpAST>(op,lhs,rhs));
}

void mmmpsr::MimiumDriver::add_line(AST_Ptr in){
   mainast.addAST(in);
}

std::ostream&  mmmpsr::MimiumDriver::print( std::ostream &stream )
{
   stream << red  << "Results: " << norm << "\n";
   stream << blue << "Words: " << norm << mainast.to_string() << "\n";
   return(stream);
}

