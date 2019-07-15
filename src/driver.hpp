#pragma once
#include <memory>
#include <string>

#include "mimium_parser.hpp"
#include "op_map.hpp"
#include "scanner.hpp"

template <class T>
AST_Ptr createAST_Ptr(T inputs) {
  return std::dynamic_pointer_cast<AST>(std::make_unique<T>(inputs));
}
using tokentype = mmmpsr::MimiumParser::token::yytokentype;

namespace mmmpsr {

class MimiumDriver {
 public:
  MimiumDriver() { mainast = std::make_shared<ListAST>(); };
  MimiumDriver(std::string cwd) :  working_directory(cwd) { 
    mainast = std::make_shared<ListAST>(); 
    };

  virtual ~MimiumDriver();
  void parse(std::istream &is);
  void parsestring(const std::string str);
  void parsefile(const std::string filename);
  void clear();
  void setWorkingDirectory(const std::string cwd);

  AST_Ptr add_number(double num);
  AST_Ptr add_symbol(std::string str);

  AST_Ptr add_op(std::string op, AST_Ptr lhs, AST_Ptr rhs);

  AST_Ptr add_arguments(AST_Ptr arg);
  AST_Ptr add_lambda(AST_Ptr args, AST_Ptr body);

  AST_Ptr add_fcall(AST_Ptr fname, AST_Ptr args);
  AST_Ptr add_declaration(std::string fname, AST_Ptr args);

  AST_Ptr add_array(AST_Ptr array);
  AST_Ptr add_array_access(
      AST_Ptr array,
      AST_Ptr index);  // todo: is it better to use fcall as syntax sugar?

  AST_Ptr add_return(AST_Ptr expr);
  AST_Ptr add_assign(AST_Ptr symbol, AST_Ptr expr);
  AST_Ptr add_statements(AST_Ptr statements);

  AST_Ptr add_fdef(AST_Ptr fname, AST_Ptr args, AST_Ptr statements);

  AST_Ptr add_if(AST_Ptr condition, AST_Ptr thenstatement,
                 AST_Ptr elsestatement);

  AST_Ptr add_forloop(AST_Ptr var, AST_Ptr iterator, AST_Ptr expression);

  AST_Ptr set_time(AST_Ptr elem, AST_Ptr time);
  // void add_line(AST_Ptr in); // final action
  void add_top(AST_Ptr top);  // final action

  std::ostream &print(std::ostream &stream);
  AST_Ptr getMainAst() { return mainast; };

 private:
  std::shared_ptr<ListAST> mainast;
  std::unique_ptr<AST> temporaryast;

  std::unique_ptr<mmmpsr::MimiumParser> parser = nullptr;
  std::unique_ptr<mmmpsr::MimiumScanner> scanner = nullptr;
  std::string working_directory = "";
  const std::string red = "\033[1;31m";
  const std::string blue = "\033[1;36m";
  const std::string norm = "\033[0m";
};
};  // namespace mmmpsr