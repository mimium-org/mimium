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
   
   LBRACE "{"
   RBRACE "}"

   ARROW "->"
   FUNC "fn"
   IF "if"
   ELSE "ELSE"

   END "end_token"
   RETURN "return_token"

   ENDFILE    0     "end of file"
   NEWLINE "newline"
;
%token <double> NUM "number_token"
%token  <std::string> SYMBOL "symbol_token"
%token  <std::string> FNAME "fname_token"




%type <AST_Ptr> fname "fname"

%type <AST_Ptr> num "number"
%type <AST_Ptr> symbol "symbol"

%type <AST_Ptr> single "symbol or number"

%type <AST_Ptr> expr "expression"
%type <AST_Ptr> term_time "term @ something"
%type <AST_Ptr> term "primary"

%type <AST_Ptr> lambda "lambda"

%type <AST_Ptr> arguments_top "arguments top"

%type <AST_Ptr> arguments "arguments for fdef"
%type <AST_Ptr> arguments_fcall "arguments for fcall"



%type <AST_Ptr> assign "assign"
%type <AST_Ptr> fdef "fdef"

%type <AST_Ptr> fcall "fcall"


%type <AST_Ptr> statement "single statement"
%type <AST_Ptr> statements "statements"
%type <AST_Ptr> block "block"


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
block: LBRACE  statements RBRACE {$$ = $$ = std::move($2);};

opt_nl: {}
      | NEWLINE {};

statement : assign {$$=std::move($1);} 
         | fdef  {$$=std::move($1);} 
         |RETURN expr {$$ = driver.add_return(std::move($2));}
         ;

fdef : FUNC symbol arguments_top block {$$ = driver.add_assign(std::move($2),driver.add_lambda(std::move($3),std::move($4)));};




/* end : END; */

lambda: arguments_top ARROW LBRACE expr RBRACE {$$ = driver.add_lambda(std::move($1),std::move($4));};

assign : symbol ASSIGN expr {$$ = driver.add_assign(std::move($1),std::move($3));}
      |  symbol ASSIGN lambda {$$ = driver.add_assign(std::move($1),std::move($3));}
;

arguments_top: '(' arguments ')' {$$=std::move($2);};

arguments : symbol ',' arguments   {$3->addAST(std::move($1));
                                    $$ = std::move($3); }
         |  symbol {$$ = driver.add_arguments(std::move($1));}
         ;


arguments_fcall : single ',' arguments_fcall   {$3->addAST(std::move($1));
                                          $$ = std::move($3); }
         | single {$$ = driver.add_arguments(std::move($1));}
         ;

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

term_time : term AT term {$$ = driver.set_time(std::move($1),std::move($3));}
         | term {$$ = std::move($1);}
         ;
term : single
      |fcall
      | '(' expr ')' {$$ =std::move($2);};




fcall : symbol '(' arguments_fcall ')' {$$ = driver.add_fcall(std::move($1),std::move($3));}
;

single : symbol{$$=std::move($1);}
      |  num   {$$=std::move($1);};

num :NUM {$$ = driver.add_number($1);};


symbol : SYMBOL {$$ = driver.add_symbol($1);}

%%


void 
mmmpsr::MimiumParser::error( const location_type &l, const std::string &err_message )
{
   std::cerr << "Error: " << err_message << " at " << l << "\n";
}