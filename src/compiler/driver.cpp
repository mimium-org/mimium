#include <iostream>
#include <fstream>

#include <memory>
#include <string>
#include <sstream>

#include "compiler/driver.hpp"

#ifndef MIMIUM_DEBUG
#define DEBUG_LEVEL 0
#endif

namespace mmmpsr{

MimiumDriver::~MimiumDriver() = default;
void MimiumDriver::parse(std::istream &is){
   scanner.reset();
   scanner = std::make_unique<mmmpsr::MimiumScanner>( is );
   parser.reset();
   parser = std::make_unique<mmmpsr::MimiumParser>( *scanner,*this );
   parser->set_debug_level(DEBUG_LEVEL); //debug
   parser->parse();
}
void MimiumDriver::parsestring(const std::string str){
   std::stringbuf str_buf( str );
   std::istream instream( &str_buf );
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
   mainast = std::make_shared<ListAST>();
}

void MimiumDriver::setWorkingDirectory(const std::string str){
   working_directory = str;
}

AST_Ptr MimiumDriver::add_number(double num){
   return std::make_unique<NumberAST>(std::move(num));
}

std::shared_ptr<LvarAST> MimiumDriver::add_lvar(std::string str){
   return std::make_unique<LvarAST>(std::move(str));
}
std::shared_ptr<LvarAST> MimiumDriver::add_lvar(std::string str, mimium::types::Value type){
      return std::make_unique<LvarAST>(std::move(str),std::move(type));
};


std::shared_ptr<RvarAST> MimiumDriver::add_rvar(std::string str){
   return std::make_unique<RvarAST>(std::move(str));
}

AST_Ptr MimiumDriver::add_op( std::string op,AST_Ptr lhs,AST_Ptr rhs){
   return std::make_unique<OpAST>(op,std::move(lhs),std::move(rhs));
}


std::shared_ptr<AssignAST> MimiumDriver::add_assign(std::shared_ptr<LvarAST> symbol,AST_Ptr expr){
   return std::make_unique<AssignAST>(std::move(symbol),std::move(expr));
}

std::shared_ptr<FcallArgsAST> MimiumDriver::add_fcallargs(AST_Ptr arg){
   return std::make_unique<FcallArgsAST>(std::move(arg));
}
  std::shared_ptr<FcallArgsAST> MimiumDriver::add_fcallargs(){
   return std::make_unique<FcallArgsAST>();
  }

std::shared_ptr<ArgumentsAST> MimiumDriver::add_arguments(std::shared_ptr<LvarAST> arg){
   return std::make_unique<ArgumentsAST>(std::move(arg));
}
std::shared_ptr<ArgumentsAST> MimiumDriver::add_arguments(){
   return std::make_unique<ArgumentsAST>();
}


AST_Ptr MimiumDriver::add_lambda(std::shared_ptr<ArgumentsAST> args,AST_Ptr body,mimium::types::Value type){
   return std::make_unique<LambdaAST>(std::move(args),std::move(body),std::move(type));
}
AST_Ptr MimiumDriver::add_lambda_only_with_returntype(std::shared_ptr<ArgumentsAST> args,AST_Ptr body,mimium::types::Value rettype){
   mimium::types::Function ftype(std::move(rettype),{});
   return std::make_unique<LambdaAST>(std::move(args),std::move(body),std::move(ftype));
}



std::shared_ptr<FcallAST> MimiumDriver::add_fcall(std::shared_ptr<AST> fname,std::shared_ptr<FcallArgsAST> args){
   return std::make_unique<FcallAST>(std::move(fname),std::move(args));
}
std::shared_ptr<FcallAST> MimiumDriver::add_fcall(std::shared_ptr<AST> fname,std::shared_ptr<AST> term){
      auto a = std::make_unique<FcallArgsAST>(std::move(term));
   return std::make_unique<FcallAST>(std::move(fname),std::move(a));
};

std::shared_ptr<ArrayAST> MimiumDriver::add_array(AST_Ptr array){
   return std::make_unique<ArrayAST>(std::move(array));
};
std::shared_ptr<ArrayAccessAST> MimiumDriver::add_array_access(std::shared_ptr<RvarAST> array,AST_Ptr index){
   return std::make_unique<ArrayAccessAST>(std::move(array),std::move(index));
};

AST_Ptr MimiumDriver::add_return(AST_Ptr expr){
   return std::make_unique<ReturnAST>(std::move(expr));
};

std::shared_ptr<ListAST> MimiumDriver::add_statements(AST_Ptr statements){
   return std::make_unique<ListAST>(std::move(statements));
};

AST_Ptr MimiumDriver::add_declaration( std::string fname,AST_Ptr args){
   auto fnameast = std::make_unique<LvarAST>(std::move(fname));
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
  stream << mainast->toString();
   return(stream);
}

std::ostream&  MimiumDriver::printJson( std::ostream &stream )
{
  stream << mainast->toJson();
   return(stream);
}

} // namespace mmmpsr