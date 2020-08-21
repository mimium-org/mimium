/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
 
%skeleton "lalr1.cc"
%require "3.3"
%debug 



%defines

%define api.parser.class {MimiumParser}
%define api.namespace{mimium}
%define api.location.type {ast::SourceLoc}

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
  #include "compiler/ast_loader.hpp"
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


%type <ast::Number> num "number"
%type <ast::Lvar> lvar "left value"

%type <ast::Rvar> rvar "right value"
%type <ast::Self> self "self"

%type <ast::String> string "string" 

%type <ast::Fcall> now "now"


%type <ast::ExprPtr> single "symbol or number"
%type <ast::Op> op "operator"

%type <ast::ExprPtr> expr "expression"
// %type <AST_Ptr> term_time "term @ something"
%type <ast::ExprPtr> term "primary"

%type <ast::Lambda> lambda "lambda"
%type <ast::LambdaArgs> arguments_top "arguments top"
%type <std::deque<ast::Lvar>> arguments "arguments for fdef"

// %type <AST_Ptr> declaration "declaration"
// %type <AST_Ptr> include "include declaration"


%type <ast::FcallArgs> fcallargs "arguments for fcall"
%type <ast::Fcall> fcall "fcall"
//expression version of if(syntax sugar to calling special function)
%type <ast::Fcall> ifexpr "if expr"

%type <ast::Time> fcalltime "fcall with time"


%type <ast::ArrayInit> array "array"
%type <std::deque<ast::ExprPtr>> arrayelems "arrayelems"


%type <ast::ArrayAccess> array_access "array access"


%type <ast::Assign> assign "assign"
// Syntax Sugar 
%type <ast::Assign> fdef "fdef"


%type <ast::If> ifstatement "if statement"
%type <ast::For> forloop "forloop"


%type <std::shared_ptr<ast::Statement>> statement "single statement"

%type <ast::Statements> statements "statements"
%type <ast::Statements> top "top"
%type <ast::Statements> block "block"





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
            $$ = ast::String{{{@1,$1} ,$1}};}

num:  NUM {
            $$ = ast::Number{{@1 ,std::to_string($1)} ,$1};}


lvar: SYMBOL TYPE_DELIM types {
            $$ = ast::Lvar{ { {@$,$1},$1,}, std::move($3)};}
      |SYMBOL {
            $$ = ast::Lvar{ { {@$,$1},$1,}, std::nullopt };}

rvar: SYMBOL {
            @$ = @1;
            $$ = ast::Rvar{{{@$,$1} ,$1}};}

self: SELF {
            @$ = @1;
            $$ = ast::Self{@$,"self"};}

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
       self   {$$=ast::makeExpr($1);}
      |now    {$$=ast::makeExpr($1);}
      |rvar   {$$=ast::makeExpr($1);}
      |string {$$=ast::makeExpr($1);}
      |num    {$$=ast::makeExpr($1);}

// function call ()

fcallargs: 
       fcallargs ',' expr  
                { $1.args.push_back(std::move($3));
                  $$ = std::move($1); }
      |expr   {$$ = ast::FcallArgs{ {@1,""}, {std::move($1)} };}
      |%empty {$$ = ast::FcallArgs{ {}, {} };}

fcall: term '(' fcallargs ')' {$$ = ast::Fcall{{@$,""},std::move($1),std::move($3)};}

fcalltime: fcall AT term      {$$ = ast::Time{{@$,""},std::move($1),std::move($3)};;}
      
// now: syntax sugar to fcall;
now: NOW { 
      $$ = ast::Fcall{{@1,"now"},
                         ast::makeExpr(ast::Rvar{ {{ @1,"now"} ,"mimium_getnow"}}),
                         ast::FcallArgs{ { @1,"now"},{}}};
      }

// lambda

lambda: OR arguments OR block {
      auto args = ast::LambdaArgs{{@2,"args"},std::move($2)};
      $$ = ast::Lambda{{@$,"lambda"} ,std::move(args),std::move($4),std::nullopt};}
       |OR arguments OR ARROW types block {
      auto args = ast::LambdaArgs{{@2,"args"},std::move($2)};
      $$ = ast::Lambda{{@$,"lambda"},std::move(args),std::move($6),std::move($5)};}

ifexpr: IF '(' expr ')' expr ELSE expr{$$ = ast::Fcall{{@1,"now"},
                         ast::makeExpr(ast::Rvar{{{@$,"ifexpr"},"ifexpr"}}),
                         ast::FcallArgs{{@$,"ifexpr"},{std::move($3),std::move($5),std::move($7)}}};}


// array initialization

array: '[' arrayelems ']' { 
      // @$ = {@1.first_line,@1.first_col,@3.last_line,@3.last_col};
      $$ = ast::ArrayInit{{@$,"array"} ,std::move($2)};}

arrayelems: single ',' arrayelems   {
                                    $3.push_front(std::move($1));
                                    $$ = std::move($3); }
         |  single {$$ = std::deque<ast::ExprPtr>{std::move($1)};}

// array access
array_access: term '[' term ']' {
      // @$ = {@1.first_line,@1.first_col,@4.last_line,@4.last_col};
      $$ = ast::ArrayAccess{{@$,"arrayaccess"},std::move($1),std::move($3)};}


op:   expr ADD    expr   {$$ = ast::Op{{@$,"op"},ast::OpId::Add,        std::move($1),std::move($3)};}
     |expr SUB    expr   {$$ = ast::Op{{@$,"op"},ast::OpId::Sub,        std::move($1),std::move($3)};}
     |expr MUL    expr   {$$ = ast::Op{{@$,"op"},ast::OpId::Mul,        std::move($1),std::move($3)};}
     |expr DIV    expr   {$$ = ast::Op{{@$,"op"},ast::OpId::Div,        std::move($1),std::move($3)};}
     |expr MOD    expr   {$$ = ast::Op{{@$,"op"},ast::OpId::Mod,        std::move($1),std::move($3)};}
     |expr EXPONENT expr {$$ = ast::Op{{@$,"op"},ast::OpId::Exponent,   std::move($1),std::move($3)};}
     |expr OR     expr   {$$ = ast::Op{{@$,"op"},ast::OpId::Or,         std::move($1),std::move($3)};}
     |expr AND    expr   {$$ = ast::Op{{@$,"op"},ast::OpId::And,        std::move($1),std::move($3)};}
     |expr BITOR  expr   {$$ = ast::Op{{@$,"op"},ast::OpId::BitOr,      std::move($1),std::move($3)};}
     |expr BITAND expr   {$$ = ast::Op{{@$,"op"},ast::OpId::BitAnd,     std::move($1),std::move($3)};}
     |expr GT expr       {$$ = ast::Op{{@$,"op"},ast::OpId::GreaterThan,std::move($1),std::move($3)};}
     |expr LT expr       {$$ = ast::Op{{@$,"op"},ast::OpId::LessThan,   std::move($1),std::move($3)};}
     |expr GE expr       {$$ = ast::Op{{@$,"op"},ast::OpId::GreaterEq,  std::move($1),std::move($3)};}
     |expr LE expr       {$$ = ast::Op{{@$,"op"},ast::OpId::LessEq,     std::move($1),std::move($3)};}
     |expr LSHIFT expr   {$$ = ast::Op{{@$,"op"},ast::OpId::LShift,     std::move($1),std::move($3)};}
     |expr RSHIFT expr   {$$ = ast::Op{{@$,"op"},ast::OpId::RShift,     std::move($1),std::move($3)};}
     |expr NEQ expr      {$$ = ast::Op{{@$,"op"},ast::OpId::NotEq,      std::move($1),std::move($3)};}
     |SUB expr           {$$ = ast::Op{{@$,"op"},ast::OpId::Sub,        std::nullopt, std::move($2)};}
     |NOT expr           {$$ = ast::Op{{@$,"op"},ast::OpId::Not,        std::nullopt, std::move($2)};}


expr: op  {$$ = ast::makeExpr($1);}
     |expr PIPE expr     {
                        auto arg = ast::FcallArgs{{@1,"pipe"},{std::move($1)}};
                        auto fcall = ast::Fcall{{@$,"pipe"},std::move($3),std::move(arg)};
                         $$ = ast::makeExpr(std::move(fcall));}
     |term {$$ = std::move($1);}

term: fcalltime     {$$ = ast::makeExpr($1);}
      |    fcall     {$$ = ast::makeExpr($1);}
      |    array     {$$ = ast::makeExpr($1);}
      | array_access {$$ = ast::makeExpr($1);}
      |    lambda    {$$ = ast::makeExpr($1);}
      |    ifexpr    {$$ = ast::makeExpr($1);}
      |    single    {$$ = std::move($1);}
      | '(' expr ')' {$$ = std::move($2);}


// Statements 

// Assign statement 

assign : lvar ASSIGN expr {$$ = ast::Assign{{@$,"assign"},std::move($1),std::move($3)};}


// function definition (syntax sugar to assignment of lambda function to variable)

arguments_top: '(' arguments ')' {$$=ast::LambdaArgs{{@$,"largs"},std::move($2)};}

arguments: lvar ',' arguments {$3.push_front(std::move($1));
                               $$ = std::move($3); }
         | lvar   {$$ = std::deque<ast::Lvar>{std::move($1)};}
         | %empty {$$ = {};}

fdef: FUNC lvar arguments_top block {
      auto lambda = ast::Lambda{{@$,"lambda"} ,std::move($3),std::move($4),std::nullopt};
      $$ = ast::Assign{{@$,"fdef"},std::move($2),ast::makeExpr(lambda)};}
      |FUNC lvar arguments_top ARROW types block {
      auto lambda = ast::Lambda{{@$,"lambda"} ,std::move($3),std::move($6),std::move($5)};
      $$ = ast::Assign{{@$,"fdef"},std::move($2),ast::makeExpr(lambda)};}


top: opt_nl statements opt_nl ENDFILE {driver.setTopAst(std::make_shared<ast::Statements>(std::move($2)));}

statements: statement{  $$ = std::deque<std::shared_ptr<ast::Statement>>{std::move($1)};}
            |statements NEWLINE statement {$1.push_back(std::move($3));
                                          $$= std::move($1);  }
            
      
block: LBRACE  NEWLINE statements opt_nl RBRACE {$$ = std::move($3);}
// for one liner statement
      |LBRACE expr RBRACE        {auto stmt =ast::makeStatement(std::move($2));
                                  $$= ast::Statements{std::move(stmt)};}
      |LBRACE RETURN expr RBRACE {auto ret = ast::Return{{@$,"ret"},std::move($3)};
                                  auto stmt =ast::makeStatement(std::move(ret));
                                  $$= ast::Statements{std::move(stmt)};}

opt_nl:%empty
      | NEWLINE {}

statement: assign       {$$=ast::makeStatement(std::move($1));} 
          |fdef         {$$=ast::makeStatement(std::move($1));}
          |ifstatement  {$$=ast::makeStatement(std::move($1));}
          |forloop      {$$=ast::makeStatement(std::move($1));}
      //     |declaration  {$$=ast::makeStatement(std::move($1));}
          |RETURN expr  {auto ret = ast::Return{{@$,"ret"},std::move($2)};
                         $$=ast::makeStatement(std::move(ret));}
          |expr         {$$=ast::makeStatement(std::move($1));}


ifstatement: IF '(' expr ')' block {$$ = ast::If{{@$,"if"},std::move($3),std::move($5),std::nullopt};}
            |IF '(' expr ')' block ELSE block {$$ = ast::If{{@$,"if"},std::move($3),std::move($5),std::move($7)};}

forloop: FOR '(' lvar IN expr ')' block {$$ = ast::For{{@$,"for"},std::move($3),std::move($5),std::move($7)};};


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