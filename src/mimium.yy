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


%}

%code requires{
   namespace mmmpsr {
      class MimiumDriver;
      class MimiumScanner;
   }
  #include <memory>
  #include <sstream>
#include "ast.hpp"
#include "helper_functions.hpp"
  using AST_Ptr = std::shared_ptr<AST>;
  #define YYDEBUG 1

}
%parse-param { MimiumScanner &scanner  }
%parse-param { MimiumDriver  &driver  }

%code {
    #include "driver.hpp"

  #undef yylex
  #define yylex scanner.yylex
}
%define api.value.type variant
%define parse.assert
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
   
   ASSIGN "="
   AT "@"
   
   LBRACE "{"
   RBRACE "}"

   ARROW "->"
   FUNC "fn"
   IF "if"
   ELSE "ELSE"

   FOR "for"
   IN "in"

   TYPE_DELIM "::"
   TYPEFLOAT "float_typetoken"
   TYPEFN "fn_typetoken"

   INCLUDE "include_token"

   /*END "end_token"*/
   RETURN "return_token"

   ENDFILE    0     "end of file"
   NEWLINE "newline"
;
%token <double> NUM "number_token"
%token  <std::string> SYMBOL "symbol_token"
%token  <std::string> STRING "string_token"
%token  <std::string> FNAME "fname_token"


%type <mimium::types::Value> types "types"
%type <mimium::types::Value> fntype "fn_type"
%type <std::vector<mimium::types::Value>> fntype_args "fntype_args"



%type <AST_Ptr> num "number"
%type <AST_Ptr> lvar "left value"
%type <AST_Ptr> rvar "right value"
%type <AST_Ptr> string "string"

%type <AST_Ptr> single "symbol or number"

%type <AST_Ptr> expr "expression"
%type <AST_Ptr> term_time "term @ something"
%type <AST_Ptr> term "primary"

%type <AST_Ptr> lambda "lambda"

%type <AST_Ptr> declaration "declaration"
%type <AST_Ptr> include "include declaration"

%type <AST_Ptr> arguments_top "arguments top"

%type <AST_Ptr> arguments "arguments for fdef"
%type <AST_Ptr> arguments_fcall "arguments for fcall"

%type <AST_Ptr> array "array"
%type <AST_Ptr> array_elems "array elements"
%type <AST_Ptr> array_access "array access"


%type <AST_Ptr> assign "assign"
%type <AST_Ptr> fdef "fdef"

%type <AST_Ptr> fcall "fcall"

%type <AST_Ptr> ifstatement "if statement"
%type <AST_Ptr> statement "single statement"
%type <AST_Ptr> statements "statements"
%type <AST_Ptr> block "block"

%type <AST_Ptr> forloop "forloop"

%type <AST_Ptr> top "top"


%locations


%left  OR BITOR
%left  AND BITAND
%nonassoc  EQ NEQ
%left  ADD SUB
%left  MUL DIV MOD
%left  EXPONENT


%left  AT

%right NOT 

%left ','

%left NEWLINE 


%start top

%%

top :statements ENDFILE {driver.add_top(std::move($1));}
    ;

statements :statement NEWLINE statements {$3->addAST(std::move($1));
                                           $$ = std::move($3);  }
            | statement opt_nl{  $$ = driver.add_statements(std::move($1));}
      ;
block: LBRACE  opt_nl statements opt_nl RBRACE {$$ = $$ = std::move($3);};

opt_nl: {}
      | NEWLINE {};

statement : assign {$$=std::move($1);} 
         | fdef  {$$=std::move($1);} 
         | ifstatement  {$$=std::move($1);} 
         | forloop {$$=std::move($1);}
         | declaration {$$=std::move($1);} 
         |RETURN expr {$$ = driver.add_return(std::move($2));}
         | expr {$$ = std::move($1);}//for void function
         ;

fdef : FUNC lvar arguments_top block {$$ = driver.add_assign(std::move($2),driver.add_lambda(std::move($3),std::move($4)));}
      |FUNC lvar arguments_top ARROW types block {$$ = driver.add_assign(std::move($2),driver.add_lambda(std::move($3),std::move($6),std::move($5)));};

ifstatement: IF '(' expr ')' block {$$ = driver.add_if(std::move($3),std::move($5),nullptr);}
            |IF '(' expr ')' block ELSE block {$$ = driver.add_if(std::move($3),std::move($5),std::move($7));}
;

forloop: FOR lvar IN expr block {$$ = driver.add_forloop(std::move($2),std::move($4),std::move($5));};

/* end : END; */

lambda: arguments_top ARROW block {$$ = driver.add_lambda(std::move($1),std::move($3));};

assign : lvar ASSIGN expr {$$ = driver.add_assign(std::move($1),std::move($3));}
;

arguments_top: '(' arguments ')' {$$=std::move($2);};

arguments : lvar ',' arguments   {$3->addAST(std::move($1));
                                    $$ = std::move($3); }
         |  lvar {$$ = driver.add_arguments(std::move($1));}
         ;


arguments_fcall : expr ',' arguments_fcall   {$3->addAST(std::move($1));
                                          $$ = std::move($3); }
         | expr {$$ = driver.add_arguments(std::move($1));}
         ;

expr : expr ADD    expr  {$$ = driver.add_op("+" , std::move($1),std::move($3));}
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
     | term_time {$$ = std::move($1);};

term_time : term AT term {$$ = driver.set_time(std::move($1),std::move($3));}
         | term {$$ = std::move($1);}
         ;
term : single
      |fcall
      |array
      |array_access
      |lambda
      | '(' expr ')' {$$ =std::move($2);};

declaration : include {$$=std::move($1);} 
;

include : INCLUDE '(' arguments_fcall ')' {$$ = driver.add_declaration("include",std::move($3)); }
;

fcall : rvar '(' arguments_fcall ')' {$$ = driver.add_fcall(std::move($1),std::move($3));}
;

array : '[' array_elems ']' {$$ = std::move($2);}

array_elems : single ',' array_elems   {$3->addAST(std::move($1));
                                    $$ = std::move($3); }
         |  single {$$ = driver.add_array(std::move($1));}
         ;
array_access: rvar '[' term ']' {$$ = driver.add_array_access(std::move($1),std::move($3));}; 

single : rvar{$$=std::move($1);}
      | string{$$=std::move($1);}
      |  num   {$$=std::move($1);};

string : STRING {$$ = driver.add_lvar($1);}
;

num :NUM {$$ = driver.add_number($1);};


lvar : SYMBOL {$$ = driver.add_lvar($1);}
      |SYMBOL TYPE_DELIM types {$$ = driver.add_lvar($1,$3);};

rvar : SYMBOL {$$ = driver.add_rvar($1);}

types : TYPEFLOAT {
      mimium::types::Float f;
      $$ =std::move(f);}
      | fntype;

fntype: TYPEFN '(' fntype_args ')' ARROW types {
      mimium::types::Function f;
      f.init(std::move($3),std::move($6));
      mimium::types::Value v = std::move(f);
      $$ = std::move(v);
      };

fntype_args  :  fntype_args ',' types {
                  $1.push_back($3);
                  $$ = std::move($1);}
            |   types  {
                  std::vector<mimium::types::Value> v= {$1};
                  $$ = std::move(v);}
; 

%%


void 
mmmpsr::MimiumParser::error( const location_type &l, const std::string &err_message )
{
      std::stringstream ss;
      ss  <<  "Parse Error: " << err_message << " at " << l << "\n";
mimium::Logger::debug_log(ss.str(),mimium::Logger::ERROR);
}