#include <iostream>
#include <fstream>

#include <string>
#include <sstream>

#include "driver.hpp"

#ifndef MIMIUM_DEBUG
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
void MimiumDriver::parsestring(const std::string str){
   std::stringbuf strBuf( str.c_str() );
   std::istream instream( &strBuf );
   parse(instream);
}

void MimiumDriver::parsefile(const std::string filename){
   std::ifstream ifs;
   ifs.exceptions(std::fstream::failbit | std::fstream::badbit);

   try{
      if(filename[0] == '/'){ // check if absolute path or not
         ifs.open(filename);
      }else{
         ifs.open(working_directory + filename);
      }
      parse(ifs);
   }catch( std::ios_base::failure e){
        std::cerr << "File read error" << std::endl;
        if(ifs.fail()){
         std::cerr << "no such file:" << filename <<std::endl;
        }
   }
}

void MimiumDriver::clear(){
   mainast.reset(new ListAST());
}

void MimiumDriver::setWorkingDirectory(const std::string str){
   working_directory = str;
}

AST_Ptr MimiumDriver::add_number(double num){
   return std::make_unique<NumberAST>(std::move(num));
}

AST_Ptr MimiumDriver::add_symbol(std::string str){
   return std::make_unique<SymbolAST>(std::move(str));
}

AST_Ptr MimiumDriver::add_op( std::string op,AST_Ptr lhs,AST_Ptr rhs){
   return std::make_unique<OpAST>(op,std::move(lhs),std::move(rhs));
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

AST_Ptr MimiumDriver::add_array(AST_Ptr array){
   return std::make_unique<ArrayAST>(std::move(array));
};
AST_Ptr MimiumDriver::add_array_access(AST_Ptr array,AST_Ptr index){
   return std::make_unique<ArrayAccessAST>(std::move(array),std::move(index));
};

AST_Ptr MimiumDriver::add_return(AST_Ptr expr){
   return std::make_unique<ReturnAST>(std::move(expr));
};

AST_Ptr MimiumDriver::add_statements(AST_Ptr statements){
   return std::make_unique<ListAST>(std::move(statements));
};

AST_Ptr MimiumDriver::add_declaration( std::string fname,AST_Ptr args){
   auto fnameast = std::make_unique<SymbolAST>(std::move(fname));
   return std::make_unique<DeclarationAST>(std::move(fnameast) ,std::move(args));
};


AST_Ptr MimiumDriver::add_if(AST_Ptr condition,AST_Ptr thenstatement,AST_Ptr elsestatement=nullptr){
   return std::make_unique<IfAST>(std::move(condition),std::move(thenstatement),std::move(elsestatement));
};

AST_Ptr MimiumDriver::add_forloop(AST_Ptr var,AST_Ptr iterator,AST_Ptr expression){
   return std::make_unique<ForAST>(std::move(var),std::move(iterator),std::move(expression));
}

AST_Ptr MimiumDriver::set_time(AST_Ptr elem,AST_Ptr time){
    return std::make_unique<TimeAST>(std::move(elem),std::move(time));
}

// void MimiumDriver::add_line(AST_Ptr in){
//    mainast->addAST(std::move(in));
// }
void MimiumDriver::add_top(AST_Ptr top){
     mainast->addAST(std::move(top));
}

std::ostream&  MimiumDriver::print( std::ostream &stream )
{
  mainast->to_string(stream);
   return(stream);
}

} // namespace mmmpsr