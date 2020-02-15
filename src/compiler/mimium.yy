/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
 
%skeleton "lalr1.cc"
%require "3.0"
%debug 



%defines
%define api.parser.class {MimiumParser}
%define api.namespace{mmmpsr}

%token-table

%{
#define YYDEBUG 1
#define YYERROR_VERBOSE 1
#define YY_INPUT(buf,result,max_size)  {\
    result = GetNextChar(buf, max_size); \
    if (  result <= 0  ) \
      result = YY_NULL; \
    }

%}

%code requires{
   namespace mmmpsr {
      class MimiumDriver;
      class MimiumScanner;
   }
  #include <memory>
  
  #include <sstream>

#include "basic/ast.hpp"
#include "basic/helper_functions.hpp"
using namespace mimium;

  #define YYDEBUG 1

}
%parse-param { MimiumScanner &scanner  }
%parse-param { MimiumDriver  &driver  }

%code {
    #include "compiler/driver.hpp"
  using namespace mimium;

  #undef yylex
  #define yylex scanner.yylex
}
%define api.value.type variant
%define parse.assert
%define parse.error verbose
%token
   ADD "+"
   SUB "-"
   MOD "%"
   MUL "*"
   DIV "/"
   EXPONENT "^"
   AND "&"
   OR "|"
   BITAND "&&"
   BITOR "||"
   NEQ "!="
   EQ "=="
   NOT "!"
   LT "<"
   GT ">"
   LE "<="
   GE ">="
   LSHIFT "<<"
   RSHIFT ">>"
   
   NOW "now_token"

   ASSIGN "="
   AT "@"
   
   LBRACE "{"
   RBRACE "}"


   FUNC "fn"

   IF "if"
   ELSE "else"

   FOR "for"
   IN "in"

   TYPE_DELIM ":"
   TYPEFLOAT "typeid:float"
   TYPEVOID "typeid:void"
   TYPEFN "typeid:fn"

   INCLUDE "include"

   /*END "end_token"*/
   RETURN "return"

   ENDFILE    0     "end of file"
   NEWLINE "line break"
;
// %token <double> NOW "now_token"
%token <double> NUM "number_token"
%token  <std::string> SYMBOL "symbol_token"
%token <std::string>    SELF "self_token"
%token  <std::string> STRING "string_token"
%token  <std::string> FNAME "fname_token"


%type <mimium::types::Value> types "types"
%type <mimium::types::Value> type_primitive "type_primitive"


%type <mimium::types::Value> fntype "fn_type"
%type <mimium::types::Value> reftype "ref_type"



%type <std::vector<mimium::types::Value>> fntype_args "fntype_args"


%type <AST_Ptr> num "number"
%type <std::shared_ptr<LvarAST>> lvar "left value"
%type <std::shared_ptr<LvarAST>> fname "function prototype name"

%type <std::shared_ptr<RvarAST>> rvar "right value"
%type <std::shared_ptr<SelfAST>> self "self"

%type <AST_Ptr> string "string" 

%type <AST_Ptr> now "now"


%type <AST_Ptr> single "symbol or number"

%type <AST_Ptr> expr "expression"
// %type <AST_Ptr> term_time "term @ something"
%type <AST_Ptr> term "primary"

%type <AST_Ptr> lambda "lambda"

%type <AST_Ptr> declaration "declaration"
%type <AST_Ptr> include "include declaration"

%type <std::shared_ptr<ArgumentsAST>> arguments_top "arguments top"

%type <std::shared_ptr<ArgumentsAST>> arguments "arguments for fdef"
%type <std::shared_ptr<FcallArgsAST>> arguments_fcall "arguments for fcall"

%type <std::shared_ptr<ArrayAST>> array "array"
%type <std::shared_ptr<ArrayAST>> array_elems "array elements"
%type <std::shared_ptr<ArrayAccessAST>> array_access "array access"


%type <std::shared_ptr<AssignAST>> assign "assign"
%type <AST_Ptr> fdef "fdef"

%type <AST_Ptr> fcall "fcall"

%type <AST_Ptr> ifstatement "if statement"

%type <AST_Ptr> ifexpr "if expr"

%type <AST_Ptr> statement "single statement"
%type <std::shared_ptr<ListAST>> statements "statements"
%type <AST_Ptr> block "block"

%type <AST_Ptr> forloop "forloop"

%type <std::shared_ptr<AssignAST>> top "top"


%locations

%nonassoc '(' ')'

%left ELSE
%left PIPE
%left ARROW
%left LSHIFT RSHIFT
%left LE GE GT LT
%left  OR BITOR
%left  AND BITAND
%nonassoc  EQ NEQ
%left  ADD SUB
%left  MUL DIV MOD
%left  EXPONENT


%left  AT

%right NOT 

%left ','
%left '&'


%left NEWLINE 

%start top

%%

top : opt_nl statements opt_nl ENDFILE {driver.add_top(std::move($2));}
    ;

statements :statements NEWLINE statement {$1->appendAST(std::move($3));
                                           $$ = std::move($1);  }
      | statement{  $$ = driver.add_statements(std::move($1));}

            
      
block: LBRACE  NEWLINE statements opt_nl RBRACE {$$ = std::move($3);}
      |LBRACE expr RBRACE {$$ = std::move($2);}
      |LBRACE RETURN expr RBRACE {$$ = driver.add_return(std::move($3));}

opt_nl:%empty
      | NEWLINE {};

statement : assign {$$=std::move($1);} 
         | fdef  {$$=std::move($1);} 
         | ifstatement  {$$=std::move($1);} 
         | forloop {$$=std::move($1);}
         | declaration {$$=std::move($1);} 
         |RETURN expr {$$ = driver.add_return(std::move($2));}
         | expr {$$ = std::move($1);}//for void function
         ;

assign : lvar ASSIGN expr {$$ = driver.add_assign(std::move($1),std::move($3));}

fdef : FUNC fname arguments_top block {$$ = driver.add_assign(std::move($2),driver.add_lambda(std::move($3),std::move($4)));}
      |FUNC fname arguments_top ARROW types block {$$ = driver.add_assign(std::move($2),driver.add_lambda_only_with_returntype(std::move($3),std::move($6),std::move($5)));};

fname : SYMBOL {$$ = driver.add_lvar($1);}

arguments_top: '(' arguments ')' {$$=std::move($2);};

arguments : lvar ',' arguments   {$3->addAST(std::move($1));
                                    $$ = std::move($3); }
         |  lvar {$$ = driver.add_arguments(std::move($1));}
         | %empty {$$ = driver.add_arguments();}

ifstatement: IF '(' expr ')' block {$$ = driver.add_if(std::move($3),std::move($5),nullptr);}
            |IF '(' expr ')' block ELSE block {$$ = driver.add_if(std::move($3),std::move($5),std::move($7));}

forloop: FOR lvar IN expr block {$$ = driver.add_forloop(std::move($2),std::move($4),std::move($5));};


declaration : include {$$=std::move($1);} 

include : INCLUDE '(' arguments_fcall ')' {$$ = driver.add_declaration("include",std::move($3)); }



expr : expr ADD    expr  {$$ = driver.add_op("+" , std::move($1),std::move($3)); }
     | expr SUB    expr  {$$ = driver.add_op("-" , std::move($1),std::move($3));}
     | expr MUL    expr  {$$ = driver.add_op("*" , std::move($1),std::move($3));}
     | expr DIV    expr  {$$ = driver.add_op("/" , std::move($1),std::move($3));}
     | expr MOD    expr  {$$ = driver.add_op("%" , std::move($1),std::move($3));}
     | expr EXPONENT expr{$$ = driver.add_op("^" , std::move($1),std::move($3));}
     | expr OR     expr  {$$ = driver.add_op("|" , std::move($1),std::move($3));}
     | expr AND    expr  {$$ = driver.add_op("&" , std::move($1),std::move($3));}
     | expr BITOR  expr  {$$ = driver.add_op("||", std::move($1),std::move($3));}
     | expr BITAND expr  {$$ = driver.add_op("&&", std::move($1),std::move($3));}
     | expr GT expr  {$$ = driver.add_op(">", std::move($1),std::move($3));}
     | expr LT expr  {$$ = driver.add_op("<", std::move($1),std::move($3));}
     | expr GE expr  {$$ = driver.add_op(">=", std::move($1),std::move($3));}
     | expr LE expr  {$$ = driver.add_op("<=", std::move($1),std::move($3));}
     | expr LSHIFT expr  {$$ = driver.add_op("<<", std::move($1),std::move($3));}
     | expr RSHIFT expr  {$$ = driver.add_op(">>", std::move($1),std::move($3));}
     | expr NOT expr  {$$ = driver.add_op("!", std::move($1),std::move($3));}
     | SUB expr {$$ = driver.add_op("-" ,driver.add_number(0),std::move($2));}
     | expr PIPE expr {$$ = driver.add_fcall(std::move($3),std::move($1));}
     | term {$$ = std::move($1);};

// term_time : term AT term {$$ = driver.set_time(std::move($1),std::move($3));}
//          | term {$$ = std::move($1);}
//          ;
term : 
      fcall {$$ = std::move($1);}
      |array {$$ = std::move($1);}
      |array_access {$$ = std::move($1);}
      |lambda {$$ = std::move($1);}
      |ifexpr {$$ = std::move($1);}
      |single {$$ = std::move($1);}
      | '(' expr ')' {$$ =std::move($2);}


fcall : term '(' arguments_fcall ')' {$$ = driver.add_fcall(std::move($1),std::move($3));};
      |term '(' arguments_fcall ')' AT term {$$ = driver.add_fcall(std::move($1),std::move($3),std::move($6));}
      
arguments_fcall : expr ',' arguments_fcall   {$3->addAST(std::move($1));
                                          $$ = std::move($3); }
         | expr {$$ = driver.add_fcallargs(std::move($1));}
         | %empty {$$ = driver.add_fcallargs();}
         ;




array : '[' array_elems ']' {$$ = std::move($2);}

array_elems : single ',' array_elems   {$3->addAST(std::move($1));
                                    $$ = std::move($3); }
         |  single {$$ = driver.add_array(std::move($1));}
array_access: rvar '[' term ']' {$$ = driver.add_array_access(std::move($1),std::move($3));}

lambda: OR arguments OR block {$$ = driver.add_lambda(std::move($2),std::move($4));};
      |OR arguments OR expr {$$ = driver.add_lambda(std::move($2),std::move($4));};
       |OR arguments OR ARROW types block{$$=driver.add_lambda_only_with_returntype(std::move($2),std::move($6),std::move($5));};


ifexpr: IF '(' expr ')' expr ELSE expr{$$ = driver.add_if(std::move($3),std::move($5),std::move($7),true);}

single :   self{$$=std::move($1);}
            |now{$$ = std::move($1);}
            |rvar{$$=std::move($1);}
      | string{$$=std::move($1);}
      |  num {$$=std::move($1);}

now : NOW{ 
      auto name = driver.add_rvar("mimium_getnow");
      $$ = driver.add_fcall(std::move(name),driver.add_fcallargs());}

/// primitive declaration

string : STRING {$$ = driver.add_string($1);}

num :NUM {$$ = driver.add_number($1);}


lvar : SYMBOL {$$ = driver.add_lvar($1);}
      |SYMBOL TYPE_DELIM types {$$ = driver.add_lvar($1,$3);}

self : SELF {$$ = driver.add_self($1);}

rvar : SYMBOL {$$ = driver.add_rvar($1);}

// type specifiers

types : reftype{ $$=std::move($1);}
      | fntype{ $$=std::move($1);}
      | type_primitive{ $$=std::move($1);}

type_primitive : TYPEFLOAT {$$ =mimium::types::Float();}
               | TYPEVOID  {$$ = mimium::types::Void();}
reftype : types AND{
      mimium::types::Value v = mimium::types::Ref(std::move($1));
      $$ = std::move(v);
} 

fntype: '(' fntype_args ')' ARROW types {
      mimium::types::Function f(std::move($5),std::move($2));
      mimium::types::Value v = std::move(f);
      $$ = std::move(v);
      }

fntype_args  :  fntype_args ',' types {
                  $1.push_back(std::move($3));
                  $$ = std::move($1);}
            |   types  {
                  $$ = std::vector<mimium::types::Value>{$1};}

%%

void 
mmmpsr::MimiumParser::error( const location_type &l, const std::string &err_message )
{
       std::stringstream ss;
      ss  << err_message << " at " << l << "\n";
      mimium::Logger::debug_log(ss.str(),mimium::Logger::ERROR);
}