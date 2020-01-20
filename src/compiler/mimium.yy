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


   FUNC "fn"
   IF "if"
   ELSE "ELSE"

   FOR "for"
   IN "in"

   TYPE_DELIM ":"
   TYPEFLOAT "float_typetoken"
   TYPEVOID "void_typetoken"
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
%type <std::shared_ptr<LvarAST>> lvar "left value"
%type <std::shared_ptr<LvarAST>> fname "function prototype name"

%type <std::shared_ptr<RvarAST>> rvar "right value"
// %type <AST_Ptr> string "string" //temporary comment out

%type <AST_Ptr> single "symbol or number"

%type <AST_Ptr> expr "expression"
%type <AST_Ptr> term_time "term @ something"
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
%type <AST_Ptr> statement "single statement"
%type <std::shared_ptr<ListAST>> statements "statements"
%type <AST_Ptr> block "block"

%type <AST_Ptr> forloop "forloop"

%type <std::shared_ptr<AssignAST>> top "top"


%locations

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

fdef : FUNC fname arguments_top block {$$ = driver.add_assign(std::move($2),driver.add_lambda(std::move($3),std::move($4)));}
      |FUNC fname arguments_top ARROW types block {$$ = driver.add_assign(std::move($2),driver.add_lambda_only_with_returntype(std::move($3),std::move($6),std::move($5)));};

fname : SYMBOL {$$ = driver.add_lvar($1);};

ifstatement: IF '(' expr ')' block {$$ = driver.add_if(std::move($3),std::move($5),nullptr);}
            |IF '(' expr ')' block ELSE block {$$ = driver.add_if(std::move($3),std::move($5),std::move($7));}
;

forloop: FOR lvar IN expr block {$$ = driver.add_forloop(std::move($2),std::move($4),std::move($5));};

/* end : END; */

lambda: OR arguments OR block {$$ = driver.add_lambda(std::move($2),std::move($4));};
       |OR arguments OR ARROW types block{$$=driver.add_lambda_only_with_returntype(std::move($2),std::move($6),std::move($5));};

assign : lvar ASSIGN expr {$$ = driver.add_assign(std::move($1),std::move($3));}
;

arguments_top: '(' arguments ')' {$$=std::move($2);};

arguments : lvar ',' arguments   {$3->addAST(std::move($1));
                                    $$ = std::move($3); }
         |  lvar {$$ = driver.add_arguments(std::move($1));}
         | {$$ = driver.add_arguments();}
         ;


arguments_fcall : expr ',' arguments_fcall   {$3->addAST(std::move($1));
                                          $$ = std::move($3); }
         | expr {$$ = driver.add_fcallargs(std::move($1));}
         | {$$ = driver.add_fcallargs();}
         ;

fcall : term '(' arguments_fcall ')' {$$ = driver.add_fcall(std::move($1),std::move($3));};
      



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
     | SUB expr {$$ = driver.add_op("-" ,driver.add_number(0),std::move($2));}
     | expr PIPE expr {$$ = driver.add_fcall(std::move($3),std::move($1));}
     | term_time {$$ = std::move($1);};

term_time : term AT term {$$ = driver.set_time(std::move($1),std::move($3));}
         | term {$$ = std::move($1);}
         ;
term : single {$$ = std::move($1);}
      |fcall {$$ = std::move($1);}
      |array {$$ = std::move($1);}
      |array_access {$$ = std::move($1);}
      |lambda {$$ = std::move($1);}
      | '(' expr ')' {$$ =std::move($2);};

declaration : include {$$=std::move($1);} 
;

include : INCLUDE '(' arguments_fcall ')' {$$ = driver.add_declaration("include",std::move($3)); }
;


array : '[' array_elems ']' {$$ = std::move($2);}

array_elems : single ',' array_elems   {$3->addAST(std::move($1));
                                    $$ = std::move($3); }
         |  single {$$ = driver.add_array(std::move($1));}
         ;
array_access: rvar '[' term ']' {$$ = driver.add_array_access(std::move($1),std::move($3));}; 

single : rvar{$$=std::move($1);}
      // | string{$$=std::move($1);}
      |  num   {$$=std::move($1);};

// string : STRING {$$ = driver.add_rvar($1,mimium::types::String());};

num :NUM {$$ = driver.add_number($1);};


lvar : SYMBOL {$$ = driver.add_lvar($1);}
      |SYMBOL TYPE_DELIM types {$$ = driver.add_lvar($1,$3);};

rvar : SYMBOL {$$ = driver.add_rvar($1);}

types : TYPEFLOAT {;
      $$ =mimium::types::Float();}
      | TYPEVOID{
            $$ = mimium::types::Void();}
      | fntype;

fntype: '(' fntype_args ')' ARROW types {
      mimium::types::Function f(std::move($5),std::move($2));
      mimium::types::Value v = std::move(f);
      $$ = std::move(v);
      };

fntype_args  :  fntype_args ',' types {
                  $1.push_back(std::move($3));
                  $$ = std::move($1);}
            |   types  {
                  $$ = std::vector<mimium::types::Value>{$1};}
; 

%%


void 
mmmpsr::MimiumParser::error( const location_type &l, const std::string &err_message )
{
      std::stringstream ss;
      ss  <<  "Parse Error: " << err_message << " at " << l << "\n";
      throw std::logic_error(ss.str());
}