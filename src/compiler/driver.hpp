/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// NO LONGER USED...will be deleted on next release

#pragma once
#include <memory>
#include <string>
#include <fstream>
#include <iostream>
#include "basic/ast.hpp"
#include "basic/helper_functions.hpp"
#include "basic/op_map.hpp"
#include "compiler/scanner.hpp"
#include "mimium_parser.hpp"


using tokentype = mmmpsr::MimiumParser::token::yytokentype;

namespace mmmpsr {

class MimiumDriver {
 public:
  MimiumDriver() { mainast = std::make_shared<ListAST>(); };
  explicit MimiumDriver(std::string cwd)
      : working_directory(std::move(cwd)),
        mainast(std::make_shared<ListAST>()){};

  virtual ~MimiumDriver();
  void parse(std::istream &is);
  void parsestring(std::string str);
  void parsefile(const std::string &filename);
  void clear();
  void setWorkingDirectory(const std::string cwd);

  AST_Ptr add_number(double num);
    AST_Ptr add_string(std::string& val);

  std::shared_ptr<LvarAST> add_lvar(std::string str);
  std::shared_ptr<LvarAST> add_lvar(std::string str, mimium::types::Value type);

  std::shared_ptr<RvarAST> add_rvar(std::string str);
  std::shared_ptr<SelfAST> add_self(std::string str);

  AST_Ptr add_op(std::string op, AST_Ptr lhs, AST_Ptr rhs);

  std::shared_ptr<ArgumentsAST> add_arguments(std::shared_ptr<LvarAST> arg);
  std::shared_ptr<ArgumentsAST> add_arguments();

  std::shared_ptr<FcallArgsAST> add_fcallargs(AST_Ptr arg);
  std::shared_ptr<FcallArgsAST> add_fcallargs();

  AST_Ptr add_lambda(std::shared_ptr<ArgumentsAST> args, AST_Ptr body,
                     mimium::types::Value type = mimium::types::Value());
  AST_Ptr add_lambda_only_with_returntype(std::shared_ptr<ArgumentsAST> args,
                                          AST_Ptr body,
                                          mimium::types::Value rettype);

  std::shared_ptr<FcallAST> add_fcall(std::shared_ptr<AST> fname,
                                      std::shared_ptr<FcallArgsAST> args,std::shared_ptr<AST> time = nullptr);
  // mostry for pipe
  std::shared_ptr<FcallAST> add_fcall(std::shared_ptr<AST> fname,
                                      std::shared_ptr<AST> term);

  AST_Ptr add_declaration(std::string fname, AST_Ptr args);

  std::shared_ptr<ArrayAST> add_array(AST_Ptr array);
  std::shared_ptr<ArrayAccessAST> add_array_access(
      std::shared_ptr<RvarAST>,
      AST_Ptr index);  // todo: is it better to use fcall as syntax sugar?

  AST_Ptr add_return(AST_Ptr expr);
  std::shared_ptr<AssignAST> add_assign(std::shared_ptr<LvarAST> symbol,
                                        AST_Ptr expr);
  std::shared_ptr<ListAST> add_statements(AST_Ptr statements);

  AST_Ptr add_fdef(AST_Ptr fname, AST_Ptr args, AST_Ptr statements);

  AST_Ptr add_if(AST_Ptr condition, AST_Ptr thenstatement,
                 AST_Ptr elsestatement,bool isexpr=false);

  AST_Ptr add_forloop(AST_Ptr var, AST_Ptr iterator, AST_Ptr expression);

  // AST_Ptr set_time(AST_Ptr elem, AST_Ptr time);
  // void add_line(AST_Ptr in); // final action
  void add_top(AST_Ptr top);  // final action

  std::ostream &print(std::ostream &stream);
  std::ostream &printJson(std::ostream &stream);
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