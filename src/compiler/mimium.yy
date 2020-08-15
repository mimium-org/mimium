/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
 
%skeleton "lalr1.cc"
%require "3.3"
%debug 



%defines

%define api.parser.class {MimiumParser}
%define api.namespace{mimium}
%define api.location.type {newast::SourceLoc}

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
      // class MimiumDriver;
      class MimiumScanner;
   }
   namespace mimium {
      class Driver;
   }
  #include <memory>
  #include <sstream>

#include "basic/ast_new.hpp"
#include "basic/helper_functions.hpp"
using namespace mimium;

  #define YYDEBUG 1

}
%parse-param { mmmpsr::MimiumScanner &scanner  }
%parse-param { Driver  &driver  }

%code {
  using namespace mimium;
  #include "compiler/ast_loader_new.hpp"
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
   TYPESTRING "typeid:string"

   INCLUDE "include"

   /*END "end_token"*/
   RETURN "return"

   ENDFILE    0     "end of file"
   NEWLINE "line break"
;
// %token <double> NOW "now_token"
%token <mmmfloat> NUM "number_token"
%token  <std::string> SYMBOL "symbol_token"

%token  SELF "self_token"
%token  <std::string> STRING "string_token"

%type <types::Value> types "types"
%type <types::Value> primtypes "primtypes"


%type <types::Value> fntype "fn_type"
%type <types::Value> reftype "ref_type"



%type <std::vector<types::Value>> fntypeargs "fntypeargs"


%type <newast::Number> num "number"
%type <newast::Lvar> lvar "left value"

%type <newast::Rvar> rvar "right value"
%type <newast::Self> self "self"

%type <newast::String> string "string" 

%type <newast::Fcall> now "now"


%type <newast::ExprPtr> single "symbol or number"
%type <newast::Op> op "operator"

%type <newast::ExprPtr> expr "expression"
// %type <AST_Ptr> term_time "term @ something"
%type <newast::ExprPtr> term "primary"

%type <newast::Lambda> lambda "lambda"
%type <newast::LambdaArgs> arguments_top "arguments top"
%type <std::deque<newast::Lvar>> arguments "arguments for fdef"

// %type <AST_Ptr> declaration "declaration"
// %type <AST_Ptr> include "include declaration"


%type <newast::FcallArgs> fcallargs "arguments for fcall"
%type <newast::Fcall> fcall "fcall"
//expression version of if(syntax sugar to calling special function)
%type <newast::Fcall> ifexpr "if expr"

%type <newast::Time> fcalltime "fcall with time"


%type <newast::ArrayInit> array "array"
%type <std::deque<newast::ExprPtr>> arrayelems "arrayelems"


%type <newast::ArrayAccess> array_access "array access"


%type <newast::Assign> assign "assign"
// Syntax Sugar 
%type <newast::Assign> fdef "fdef"


%type <newast::If> ifstatement "if statement"
%type <newast::For> forloop "forloop"


%type <std::shared_ptr<newast::Statement>> statement "single statement"

%type <newast::Statements> statements "statements"
%type <newast::Statements> top "top"
%type <newast::Statements> block "block"





%locations

%nonassoc '(' ')'

%nonassoc '[' ']'

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

// primitive declaration

string: STRING {
            $$ = newast::String{{{@1,$1} ,$1}};}

num:  NUM {
            $$ = newast::Number{{@1 ,std::to_string($1)} ,$1};}


lvar: SYMBOL TYPE_DELIM types {
            $$ = newast::Lvar{ { {@$,$1},$1,}, std::move($3)};}
      |SYMBOL {
            $$ = newast::Lvar{ { {@$,$1},$1,}, std::nullopt };}

rvar: SYMBOL {
            @$ = @1;
            $$ = newast::Rvar{{{@$,$1} ,$1}};}

self: SELF {
            @$ = @1;
            $$ = newast::Self{@$,"self"};}

// type specifiers


primtypes: TYPEFLOAT {$$ =types::Float();}
               | TYPEVOID  {$$ = types::Void();}
               | TYPESTRING  {$$ = types::String();}

reftype: 
      types AND {
            types::Value v = types::Ref(std::move($1));
            $$ = std::move(v);} 

fntype: 
      '(' fntypeargs ')' ARROW types {
      types::Function f(std::move($5),std::move($2));
      types::Value v = std::move(f);
      $$ = std::move(v);
      }

fntypeargs:  
             fntypeargs ',' types { $1.push_back(std::move($3));
                                     $$ = std::move($1); }
            |types { $$ = std::vector<types::Value>{$1};}

types: 
        reftype        { $$=std::move($1);}
      | fntype         { $$=std::move($1);}
      | primtypes { $$=std::move($1);}

// Expression Section
// temporarily debug symbol for aggregate ast is disabled

single:   
       self   {$$=newast::makeExpr($1);}
      |now    {$$=newast::makeExpr($1);}
      |rvar   {$$=newast::makeExpr($1);}
      |string {$$=newast::makeExpr($1);}
      |num    {$$=newast::makeExpr($1);}

// function call ()

fcallargs: 
       expr ',' fcallargs
                { $3.args.push_back(std::move($1));
                  $$ = std::move($3); }
      |expr   {$$ = newast::FcallArgs{ {@1,""}, {std::move($1)} };}
      |%empty {$$ = newast::FcallArgs{ {}, {} };}

fcall: term '(' fcallargs ')' {$$ = newast::Fcall{{@$,""},std::move($1),std::move($3)};}

fcalltime: fcall AT term      {$$ = newast::Time{{@$,""},std::move($1),std::move($3)};;}
      
// now: syntax sugar to fcall;
now: NOW { 
      $$ = newast::Fcall{{@1,"now"},
                         newast::makeExpr(newast::Rvar{ {{ @1,"now"} ,"mimium_getnow"}}),
                         newast::FcallArgs{ { @1,"now"},{}}};
      }

// lambda

lambda: OR arguments OR block {
      auto args = newast::LambdaArgs{{@2,"args"},std::move($2)};
      $$ = newast::Lambda{{@$,"lambda"} ,std::move(args),std::move($4),std::nullopt};}
       |OR arguments OR ARROW types block {
      auto args = newast::LambdaArgs{{@2,"args"},std::move($2)};
      $$ = newast::Lambda{{@$,"lambda"},std::move(args),std::move($6),std::move($5)};}

ifexpr: IF '(' expr ')' expr ELSE expr{$$ = newast::Fcall{{@1,"now"},
                         newast::makeExpr(newast::Rvar{{{@$,"ifexpr"},"ifexpr"}}),
                         newast::FcallArgs{{@$,"ifexpr"},{std::move($3),std::move($5),std::move($7)}}};}


// array initialization

array: '[' arrayelems ']' { 
      // @$ = {@1.first_line,@1.first_col,@3.last_line,@3.last_col};
      $$ = newast::ArrayInit{{@$,"array"} ,std::move($2)};}

arrayelems: single ',' arrayelems   {
                                    $3.push_front(std::move($1));
                                    $$ = std::move($3); }
         |  single {$$ = std::deque<newast::ExprPtr>{std::move($1)};}

// array access
array_access: term '[' term ']' {
      // @$ = {@1.first_line,@1.first_col,@4.last_line,@4.last_col};
      $$ = newast::ArrayAccess{{@$,"arrayaccess"},std::move($1),std::move($3)};}


op:   expr ADD    expr   {$$ = newast::Op{{@$,"op"},newast::OpId::Add,        std::move($1),std::move($3)};}
     |expr SUB    expr   {$$ = newast::Op{{@$,"op"},newast::OpId::Sub,        std::move($1),std::move($3)};}
     |expr MUL    expr   {$$ = newast::Op{{@$,"op"},newast::OpId::Mul,        std::move($1),std::move($3)};}
     |expr DIV    expr   {$$ = newast::Op{{@$,"op"},newast::OpId::Div,        std::move($1),std::move($3)};}
     |expr MOD    expr   {$$ = newast::Op{{@$,"op"},newast::OpId::Mod,        std::move($1),std::move($3)};}
     |expr EXPONENT expr {$$ = newast::Op{{@$,"op"},newast::OpId::Exponent,   std::move($1),std::move($3)};}
     |expr OR     expr   {$$ = newast::Op{{@$,"op"},newast::OpId::Or,         std::move($1),std::move($3)};}
     |expr AND    expr   {$$ = newast::Op{{@$,"op"},newast::OpId::And,        std::move($1),std::move($3)};}
     |expr BITOR  expr   {$$ = newast::Op{{@$,"op"},newast::OpId::BitOr,      std::move($1),std::move($3)};}
     |expr BITAND expr   {$$ = newast::Op{{@$,"op"},newast::OpId::BitAnd,     std::move($1),std::move($3)};}
     |expr GT expr       {$$ = newast::Op{{@$,"op"},newast::OpId::GreaterThan,std::move($1),std::move($3)};}
     |expr LT expr       {$$ = newast::Op{{@$,"op"},newast::OpId::LessThan,   std::move($1),std::move($3)};}
     |expr GE expr       {$$ = newast::Op{{@$,"op"},newast::OpId::GreaterEq,  std::move($1),std::move($3)};}
     |expr LE expr       {$$ = newast::Op{{@$,"op"},newast::OpId::LessEq,     std::move($1),std::move($3)};}
     |expr LSHIFT expr   {$$ = newast::Op{{@$,"op"},newast::OpId::LShift,     std::move($1),std::move($3)};}
     |expr RSHIFT expr   {$$ = newast::Op{{@$,"op"},newast::OpId::RShift,     std::move($1),std::move($3)};}
     |expr NEQ expr      {$$ = newast::Op{{@$,"op"},newast::OpId::NotEq,      std::move($1),std::move($3)};}
     |SUB expr           {$$ = newast::Op{{@$,"op"},newast::OpId::Sub,        std::nullopt, std::move($2)};}
     |NOT expr           {$$ = newast::Op{{@$,"op"},newast::OpId::Not,        std::nullopt, std::move($2)};}


expr: op  {$$ = newast::makeExpr($1);}
     |expr PIPE expr     {
                        auto arg = newast::FcallArgs{{@1,"pipe"},{std::move($1)}};
                        auto fcall = newast::Fcall{{@$,"pipe"},std::move($3),std::move(arg)};
                         $$ = newast::makeExpr(std::move(fcall));}
     |term {$$ = std::move($1);}

term: fcalltime     {$$ = newast::makeExpr($1);}
      |    fcall     {$$ = newast::makeExpr($1);}
      |    array     {$$ = newast::makeExpr($1);}
      | array_access {$$ = newast::makeExpr($1);}
      |    lambda    {$$ = newast::makeExpr($1);}
      |    ifexpr    {$$ = newast::makeExpr($1);}
      |    single    {$$ = std::move($1);}
      | '(' expr ')' {$$ = std::move($2);}


// Statements 

// Assign statement 

assign : lvar ASSIGN expr {$$ = newast::Assign{{@$,"assign"},std::move($1),std::move($3)};}


// function definition (syntax sugar to assignment of lambda function to variable)

arguments_top: '(' arguments ')' {$$=newast::LambdaArgs{{@$,"largs"},std::move($2)};}

arguments: lvar ',' arguments {$3.push_front(std::move($1));
                               $$ = std::move($3); }
         | lvar   {$$ = std::deque<newast::Lvar>{std::move($1)};}
         | %empty {$$ = {};}

fdef: FUNC lvar arguments_top block {
      auto lambda = newast::Lambda{{@$,"lambda"} ,std::move($3),std::move($4),std::nullopt};
      $$ = newast::Assign{{@$,"fdef"},std::move($2),newast::makeExpr(lambda)};}
      |FUNC lvar arguments_top ARROW types block {
      auto lambda = newast::Lambda{{@$,"lambda"} ,std::move($3),std::move($6),std::move($5)};
      $$ = newast::Assign{{@$,"fdef"},std::move($2),newast::makeExpr(lambda)};}


top: opt_nl statements opt_nl ENDFILE {driver.setTopAst(std::make_shared<newast::Statements>(std::move($2)));}

statements: statement{  $$ = std::deque<std::shared_ptr<newast::Statement>>{std::move($1)};}
            |statements NEWLINE statement {$1.push_back(std::move($3));
                                          $$= std::move($1);  }
            
      
block: LBRACE  NEWLINE statements opt_nl RBRACE {$$ = std::move($3);}
// for one liner statement
      |LBRACE expr RBRACE        {auto stmt =newast::makeStatement(std::move($2));
                                  $$= newast::Statements{std::move(stmt)};}
      |LBRACE RETURN expr RBRACE {auto ret = newast::Return{{@$,"ret"},std::move($3)};
                                  auto stmt =newast::makeStatement(std::move(ret));
                                  $$= newast::Statements{std::move(stmt)};}

opt_nl:%empty
      | NEWLINE {}

statement: assign       {$$=newast::makeStatement(std::move($1));} 
          |fdef         {$$=newast::makeStatement(std::move($1));}
          |ifstatement  {$$=newast::makeStatement(std::move($1));}
          |forloop      {$$=newast::makeStatement(std::move($1));}
      //     |declaration  {$$=newast::makeStatement(std::move($1));}
          |RETURN expr  {auto ret = newast::Return{{@$,"ret"},std::move($2)};
                         $$=newast::makeStatement(std::move(ret));}
          |expr         {$$=newast::makeStatement(std::move($1));}


ifstatement: IF '(' expr ')' block {$$ = newast::If{{@$,"if"},std::move($3),std::move($5),std::nullopt};}
            |IF '(' expr ')' block ELSE block {$$ = newast::If{{@$,"if"},std::move($3),std::move($5),std::move($7)};}

forloop: FOR '(' lvar IN expr ')' block {$$ = newast::For{{@$,"for"},std::move($3),std::move($5),std::move($7)};};


// declaration : include {$$=std::move($1);} 
// include : INCLUDE '(' fcallargs ')' {$$ = driver.add_declaration("include",std::move($3)); }







%%

void 
MimiumParser::error( const location_type &l, const std::string &err_message )
{
       std::stringstream ss;
      ss  << err_message << " at " << l.begin.line <<  ":" << l.begin.col << " to " << l.end.line <<  ":" << l.end.col << "\n";
      mimium::Logger::debug_log(ss.str(),mimium::Logger::ERROR);
}