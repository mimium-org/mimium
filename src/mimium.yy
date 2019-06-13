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
#include "ast.hpp"
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
   ASSIGN "="
   AT "@"
   
   ARROW "->"
   
   END    0     "end of file"
   NEWLINE "newline"
;
%token <int> NUM "number"
%token  <std::string> SYMBOL "symbol_token"

%type <AST_Ptr> block "block"


%type <AST_Ptr> symbol "symbol"
%type <AST_Ptr> expr "expression"
%type <AST_Ptr> term_time "term @ something"
%type <AST_Ptr> term "primary"

%type <AST_Ptr> lambda "lambda"

%type <AST_Ptr> arg "arg element"
%type <AST_Ptr> arguments "arguments"

%type <AST_Ptr> assign "assign"
%type <AST_Ptr> fdef "fdef"

%type <AST_Ptr> statement "single statement"

%type <AST_Ptr> statements "statements"

%type <AST_Ptr> top "top"


%locations


%left  OR BITOR
%left  AND BITAND
%nonassoc  EQ NEQ
%left  ADD SUB
%left  MUL DIV MOD

%left  AT

%right NOT 

%start top

%%

top :statements END {driver.add_top(std::move($1));}
    ;

block : '{' statements '}' {$$ = std::move($2);}
;

statements : statement NEWLINE statements {$3->addAST(std::move($1));
                                           $$ = std::move($3);  }
            | statement {  $$ = driver.add_statements(std::move($1));}
      ;

statement : assign {$$=std::move($1);} 
         | fdef  {$$=std::move($1);} 
         ;

fdef : symbol '(' arguments ')' ASSIGN expr {$$ = driver.add_assign(std::move($1),driver.add_lambda(std::move($3),std::move($6)));};

lambda: '(' arguments ')' ARROW '{' expr '}' {$$ = driver.add_lambda(std::move($2),std::move($6));};

assign : symbol ASSIGN expr {$$ = driver.add_assign(std::move($1),std::move($3));}
      |  symbol ASSIGN lambda {$$ = driver.add_assign(std::move($1),std::move($3));}

arguments : arg ',' arguments   {$3->addAST(std::move($1));
                                 $$ = std::move($3); }
         |  arg {$$ = driver.add_arguments(std::move($1));}
         ;

arg : symbol{$$ = std::move($1);};

expr : expr ADD    expr  {$$ = driver.add_op(token::ADD , std::move($1),std::move($3));}
     | expr SUB    expr  {$$ = driver.add_op(token::SUB , std::move($1),std::move($3));}
     | expr MUL    expr  {$$ = driver.add_op(token::MUL , std::move($1),std::move($3));}
     | expr DIV    expr  {$$ = driver.add_op(token::DIV , std::move($1),std::move($3));}
     | expr MOD    expr  {$$ = driver.add_op("%" , std::move($1),std::move($3));}
     | expr EXPONENT expr{$$ = driver.add_op("^" , std::move($1),std::move($3));}
     | expr OR     expr  {$$ = driver.add_op("|" , std::move($1),std::move($3));}
     | expr AND    expr  {$$ = driver.add_op("&" , std::move($1),std::move($3));}
     | expr BITOR  expr  {$$ = driver.add_op("||", std::move($1),std::move($3));}
     | expr BITAND expr  {$$ = driver.add_op("&&", std::move($1),std::move($3));}
     | term_time {$$ = std::move($1);};

term_time : term AT NUM {$$ = driver.set_time(std::move($1),std::move($3));}
         | term {$$ = std::move($1);}
         ;
term : NUM {$$ = driver.add_number($1);}
      |symbol
        | '(' expr ')' {$$ =std::move($2);};

symbol : SYMBOL {$$ = driver.add_symbol($1);}

%%


void 
mmmpsr::MimiumParser::error( const location_type &l, const std::string &err_message )
{
   std::cerr << "Error: " << err_message << " at " << l << "\n";
}