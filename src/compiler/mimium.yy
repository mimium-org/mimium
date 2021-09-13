/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
 
%skeleton "lalr1.cc"
%require "3.3"
%debug 

%defines
%define api.parser.class {MimiumParser}
%define api.namespace{mimium}
%define api.location.type {mimium::SourceLoc}
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
#include <memory>
#include <sstream>
#include "basic/ast_new.hpp"
#include "compiler/ast_loader.hpp"
using ast = mimium::Hast;
using type_t = mimium::IType::Value;
namespace mimium{
 class MimiumScanner;
}


#define YYDEBUG 1
}
%parse-param { mimium::MimiumScanner &scanner  }
%parse-param { mimium::Driver  &driver  }

%code {
  using namespace mimium;
  #include "compiler/scanner.hpp"
  #include "compiler/ast_loader.hpp"
  #undef yylex
  #define yylex scanner.yylex
}
%define api.value.type variant
%define parse.assert /* commented out assertion because of bison bug?*/

%define parse.error verbose

%token ADD 
%token SUB 
%token MOD 
%token MUL 
%token DIV 
%token EXPONENT 
%token AND 
%token OR 
%token BITAND 
%token BITOR 
%token NEQ 
%token EQ 
%token NOT 
%token LT 
%token GT 
%token LE 
%token GE 
%token LSHIFT 
%token RSHIFT 
%token NOW 
%token ASSIGN 
%token AT 
%token LBRACE 
%token RBRACE 
%token FUNC 
%token IF 
%token ELSE 
%token FOR 
%token IN 
%token TYPE_DELIM 
%token TYPEFLOAT 
%token TYPEVOID 
%token TYPEFN
%token TYPESTRING
%token TYPEIDENT
%token INCLUDE
/* %token   END "end_token"*/
%token   RETURN
%token   ENDFILE    0     "end of file"
%token   NEWLINE "line break"


%token <mmmfloat> NUM 
%token <std::string> SYMBOL 

%token  SELF
%token  <std::string> STRING 

%token <std::string> OPERATORS

%type <type_t> types "types"

%type <List<Box<type_t>>> typeargs 
%type <List<IType::RecordKey>> strutypeargs 
%type <IType::RecordKey> strutypearg
%type <TopLevel::TypeAlias> typedecl

%type <ast::Lvar> identifier "Identifier"
%type <List<ast::Lvar>> identifierlist 
%type <List<ast::Lvar>> identifierlistproto 


%type <ast::FloatLit> num "number"
%type <ast::SelfLit> self "self"
%type <ast::StringLit> string_t "string" 
%type <ast::Symbol> symbol "symbol"

%type <ast::App> now "now"

%type <ast::Op> operators "operators"
%type <ast::Infix> infix "infix"

%type <ast::Lambda> lambda "lambda"
%type <ast::App> app "app"

%type <ast::expr> expr "expression"
%type <ast::expr> expr_non_fcall "expression other than fcall"
%type <ast::expr> expr_non_fcall_opt_ret
%type <List<Box<ast::expr>>> exprlist "list(expr)"

%type <ast::ArrayLit> array "array"
%type <ast::ArrayGet> array_access "array access"

%type <ast::TupleLit> tuple "tuple"


%type <ast::StructKey> structkey 
%type <List<ast::StructKey>> structkeylist
%type <ast::StructLit> structconstruct "struct"
%type <ast::StructGet> structaccess "struct access"

%type <ast::If> ifstmt "if"
%type <ast::expr> cond "if condition"

// %type <ast::EnvVar> envvar "envvar"

%type <ast::Block> block "block"


// Statements
%type <ast::Assignment> assign "assign"
%type <ast::LetTuple> lettuple "lettuple"
// %type <ast::App> arrayput "arrayput"

%type <ast::DefFn> fdef "fdef"

%type <ast::Schedule> schedule "fcall with time"

// %type <ast::For> forloop "forloop"


%type <ast::Statement> statement
%type <ast::Statements> statements

%type <TopLevel::Statement> topstatement 
%type <TopLevel::Expression> topstatements

%type <TopLevel::Expression> top "top"
%locations

%nonassoc COND
%left ELSE_EXPR
%nonassoc IF 
%left IF_EXPR
%left ARROW
%left INFIX
%left OPERATORS 
%left PIPE
%left LSHIFT RSHIFT
%left LE GE GT LT
%left  OR BITOR
%left  AND BITAND
%nonassoc  EQ NEQ
%left  ADD SUB
%left  MUL DIV MOD
%left  EXPONENT
%left UMINUS

%left  AT
%right NOT 
%right '('
%left  ')'
%right '['
%left  ']'
%left TUPLE
%left TUPLESINGLE

%left FCALL


%left ','
%left '&'
%left '.'

%left LIST
%nonassoc '{' '}'

%right ASSIGN
%left NEWLINE 

%start top

%%

// primitive declaration

string_t: STRING { $$ = ast::StringLit{$1,DebugInfo{@$,$1}};}
num:      NUM    { $$ = ast::FloatLit{$1,{@1 ,std::to_string($1)}};}
self:     SELF   { @$ = @1;
                   $$ = ast::SelfLit{{@$,"self"}};}

symbol: SYMBOL { @$ = @1;
                  $$ = ast::Symbol{Identifier{$1},{@$,$1}};}


identifier: SYMBOL TYPE_DELIM types {
            $$ = ast::Lvar{Identifier{$1},std::optional(std::move($3)),{@$,"declvar"}};}
      |     SYMBOL {
            $$ = ast::Lvar{Identifier{$1},std::nullopt                ,{@$,"declvar"}};}

identifierlistproto: identifierlistproto ',' identifier { $1.push_back(std::move($3));
                                          $$ = std::move($1); }%prec LIST
            |    identifier {$$ = List<ast::Lvar>{std::move($1)}; } 

identifierlist : identifierlistproto {$$ = std::move($1);}
            |    %empty              {$$ = List<ast::Lvar>{};}

typeargs:  typeargs ',' types { $1.push_back(std::move($3));
                                $$ = std::move($1); }
      |    types { $$ = List<Box<type_t>>{$1};}
      

strutypeargs : strutypeargs ',' strutypearg { $1.push_back(std::move($3));$$ = std::move($1); }
            | strutypearg {auto res = List<IType::RecordKey>{};
                        res.emplace_back(std::move($1));
                        $$ = std::move(res); }
strutypearg : SYMBOL TYPE_DELIM types { $$ = IType::RecordKey{std::move($1),std::move($3)}; }

types:  TYPEFLOAT {$$ = IType::Value{IType::Float{}};}
      | TYPEVOID  {$$ = IType::Value{IType::Unit{}};}
      | TYPESTRING{$$ = IType::Value{IType::String{}};}
      | '(' typeargs ')' ARROW types { 
            $$ = IType::Value{IType::Function{std::pair(Box(std::move($2)),Box(std::move($5)) ) }};}
      | '(' typeargs ')' { $$ = IType::Value{Box(IType::Tuple{std::move($2)})}; }
      | LBRACE strutypeargs RBRACE { $$ = IType::Value{IType::Record{std::move($2)}}; }
      | SYMBOL     { $$=makeUnknownAlias(std::move($1));}

// Type Declaration

typedecl: TYPEIDENT SYMBOL ASSIGN types {$$ = TopLevel::TypeAlias{std::move($2),std::move($4)}; }



// function call ()

exprlist:  exprlist ',' expr { $1.emplace_back(std::move($3));
                               $$ = std::move($1); }%prec LIST
          |expr              {$$ = List<Box<ast::expr>>{std::move($1)}; } 


app: expr '(' exprlist ')' {
      $$ = ast::App{std::move($1),fmap(std::move($3),[](auto&& a){ return ast::CurryArg{a};}),{@$,"app"}};}
    |expr '('          ')' {$$ = ast::App{std::move($1),{           },{@$,"app"}};} 


schedule: app AT expr      {$$ = ast::Schedule{std::move($1),std::move($3),{@$,""}};}
      
// now: syntax sugar to fcall;
now: NOW {$$ = ast::App{ast::expr{ast::Symbol{Identifier{"mimium_getnow"},{ @1,"now"}}},
                         List<ast::CurryArg>{},{@1,"now"}};}


lambda: OR identifierlist OR expr {
      $$ = ast::Lambda{ std::move($2),std::move($4),std::nullopt,{@$,"lambda"}};}

       |OR identifierlist OR ARROW types block {
      $$ = ast::Lambda{std::move($2),ast::expr{std::move($6)},std::move($5),{@$,"lambda"}};}

// array initialization

array: '[' exprlist ']' {$$ = ast::ArrayLit{std::move($2),{@$,"array"} };}
// array access
array_access: expr '[' expr ']' {$$ = ast::ArrayGet{std::move($1),std::move($3),{@$,"arrayaccess"}};}

structkey: SYMBOL TYPE_DELIM expr {$$ = ast::StructKey{std::move($1),std::move($3)}; }
structkeylist : structkeylist ',' structkey {$1.emplace_back(std::move($3));$$ = std::move($1);}
            |   structkey {$$ = List<ast::StructKey>{std::move($1)}; }

structconstruct: LBRACE structkeylist RBRACE {$$ =ast::StructLit{std::move($2),{@$,"struct"}};}

structaccess: expr '.' SYMBOL {$$ = ast::StructGet{std::move($1),std::move($3),{@$,"structget"}};}


tuple: '('  expr ',' exprlist ')' { 
      $4.push_front(std::move($2));
      $$ = ast::TupleLit{std::move($4),{@$,"tuple"}};}
      |'(' expr ',' ')' { $$ = ast::TupleLit{List<Box<ast::expr>>{std::move($2)},{@$,"tuple"}};}


operators: ADD {$$ = ast::Op{"+"};}
      |    SUB {$$ = ast::Op{"-"};}
      |    MUL {$$ = ast::Op{"*"};}
      |    DIV {$$ = ast::Op{"/"};}
      |    MOD {$$ = ast::Op{"%"};}
      |    EXPONENT  {$$ = ast::Op{"^"};}
      |    OR  {$$ = ast::Op{"|"};}
      |    AND  {$$ = ast::Op{"&"};}
      |    BITOR  {$$ = ast::Op{"||"};}
      |    BITAND  {$$ = ast::Op{"&&"};}
      |    GT  {$$ = ast::Op{">"};}
      |    LT  {$$ = ast::Op{"<"};}
      |    GE  {$$ = ast::Op{">="};}
      |    LE  {$$ = ast::Op{"<="};}
      |    LSHIFT  {$$ = ast::Op{"<<"};}
      |    RSHIFT  {$$ = ast::Op{">>"};}
      |    EQ  {$$ = ast::Op{"=="};}
      |    NEQ  {$$ = ast::Op{"!="};}
      |    PIPE {$$ = ast::Op{"|>"};}
      |    OPERATORS {$$ = ast::Op{std::move($1)};}

infix:expr operators expr{$$ = ast::Infix{std::move($2), std::move($1),std::move($3),{@$,"op"}};} %prec INFIX
     |     SUB expr     {$$ = ast::Infix{ ast::Op{"-"},ast::expr{ast::FloatLit{0,{@$,"unary_minus"}}}, std::move($2),{@$,"op"}};} %prec UMINUS
     |     NOT expr     {$$ = ast::Infix{ ast::Op{"!"},std::nullopt, std::move($2),{@$,"op"}};} %prec UMINUS



cond:  '(' expr ')'  {$$ = std::move($2);}%prec COND

//note that you should not name this terminal symbol "if"
ifstmt: IF cond expr           {$$ = ast::If{std::move($2),std::move($3),std::nullopt ,{@$,"if"}};}%prec IF_EXPR
       |IF cond expr ELSE expr {$$ = ast::If{std::move($2),std::move($3),std::move($5),{@$,"if"}};} %prec ELSE_EXPR


expr_non_fcall:infix          {$$ = ast::expr{Box(std::move($1))};}
            | array           {$$ = ast::expr{Box(std::move($1))};}
            | array_access    {$$ = ast::expr{Box(std::move($1))};}
            | structconstruct {$$ = ast::expr{Box(std::move($1))};}
            | structaccess    {$$ = ast::expr{Box(std::move($1))};}
            | tuple           {$$ = ast::expr{Box(std::move($1))};}%prec TUPLE
            | lambda          {$$ = ast::expr{Box(std::move($1))};}
            | self            {$$ = ast::expr{Box(std::move($1))};}
            | now             {$$ = ast::expr{Box(std::move($1))};}
            | symbol          {$$ = ast::expr{Box(std::move($1))};}
            | string_t        {$$ = ast::expr{Box(std::move($1))};}
            | num             {$$ = ast::expr{Box(std::move($1))};}      
           
      // | term {$$ = std::move($1);}

expr:  expr_non_fcall {$$ = std::move($1);}
      |app            {$$ = ast::expr{Box(std::move($1))};} %prec FCALL
      |ifstmt         {$$ = ast::expr{Box(std::move($1))};} %prec ELSE_EXPR
      |block          {$$ = ast::expr{Box(std::move($1))};}
       | '(' expr ')'    {$$ = std::move($2);}


// Statements 

// Assign statement 

assign   : identifier     ASSIGN expr {$$ = ast::Assignment{std::move($1),std::move($3),{@$,"assign"}};}

lettuple : identifierlist ASSIGN expr {$$= ast::LetTuple{std::move($1),std::move($3),{@$,"assign"}};}
// arrayassign : array_access ASSIGN expr {$$= ast::LetTuple{std::move($1),std::move($3),{@$,"assign"}};}

// function definition (syntax sugar to assignment of lambda function to variable)


fdef: FUNC identifier '(' identifierlist ')'             block {
      auto&& lambda = ast::Lambda{std::move($4),ast::expr{std::move($6)},std::nullopt,{@$,"lambda"}};
      $$ = ast::DefFn{std::move($2),lambda,{@$,"fdef"}};}
    | FUNC identifier '(' identifierlist ')' ARROW types block {
      auto&& lambda = ast::Lambda{std::move($4),ast::expr{std::move($8)},std::move($7),{@$,"lambda"}};
      $$ = ast::DefFn{std::move($2),lambda,{@$,"fdef"}};}


top:  topstatements opt_nl ENDFILE {driver.setTopLevel(std::move($1));}

topstatement: statement  { $$ = TopLevel::Statement{std::move($1)};}
            | typedecl    { $$ = TopLevel::Statement{std::move($1)}; }

topstatements : topstatements newlines topstatement { $1.emplace_back(std::move($3));
                                                      $$= std::move($1); }
            | opt_nl topstatement {  $$ = TopLevel::Expression{std::move($2)}; }

statements: opt_nl statement{  auto res = ast::Statements{};
                              res.v.emplace_back(std::move($2));
                              $$ = std::move(res);}
          |statements newlines statement {$1.v.emplace_back(std::move($3));
                                            $$= std::move($1);  }
            
statement: assign      {$$=ast::Statement{std::move($1)};} 
          |lettuple    {$$=ast::Statement{std::move($1)};} 
          |fdef        {$$=ast::Statement{std::move($1)};}
          |schedule    {$$=ast::Statement{std::move($1)};}
          |app         {$$=ast::Statement{std::move($1)};}
          |ifstmt      {$$=ast::Statement{std::move($1)};}


block:  LBRACE statements newlines expr_non_fcall_opt_ret opt_nl RBRACE 
                  {$$ = ast::Block{std::move($2).v,std::optional(std::move($4)),{@$,"block"}};}
      | LBRACE  opt_nl expr_non_fcall_opt_ret opt_nl RBRACE {$$ = ast::Block{List<ast::Statement>{},std::optional(std::move($3)),{@$,"block"}};}

      | LBRACE statements opt_nl RBRACE { $$ = ast::processReturn($2,@$);}
      | LBRACE opt_nl RETURN statement opt_nl RBRACE { 
            auto stmts = ast::Statements{List<ast::Statement>{$4}};
            $$ = ast::processReturn(stmts,@$);}
      | LBRACE statements opt_nl RETURN statement opt_nl RBRACE { 
            $2.v.emplace_back(std::move($5));
            $$ = ast::processReturn($2,@$);}


newlines: newlines NEWLINE
       | NEWLINE

opt_nl:%empty
      | newlines {}

expr_non_fcall_opt_ret: RETURN expr_non_fcall {$$=std::move($2);}
                        | expr_non_fcall{$$=std::move($1);}

// forloop: FOR '(' declvar IN expr ')' block {$$ = ast::For{{@$,"for"},std::move($3),std::move($5),std::move($7)};};


// declaration : include {$$=std::move($1);} 
// include : INCLUDE '(' fcallargs ')' {$$ = driver.add_declaration("include",std::move($3)); }

%%

void 
MimiumParser::error( const location_type &l, const std::string &err_message )
{
       std::stringstream ss;
      ss  << err_message << " at " << l.begin.line <<  ":" << l.begin.col << " to " << l.end.line <<  ":" << l.end.col << "\n";
      mimium::Logger::debug_log(ss.str(),mimium::Logger::ERROR_);
}