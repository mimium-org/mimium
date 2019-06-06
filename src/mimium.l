%{
  #include "scanner.hpp"
  #undef  YY_DECL
  #define YY_DECL int mmmpsr::MimiumScanner::yylex( mmmpsr::MimiumParser::semantic_type * const lval, mmmpsr::MimiumParser::location_type *loc )
  
  using token = mmmpsr::MimiumParser::token;
  #define yyterminate() return ( token::END )
  #define YY_USER_ACTION loc->step(); loc->columns(yyleng);
%}
%option c++ noyywrap nounput noinput nodefault debug
%option yyclass="mmmpsr::MimiumParser"

NONZERO  [1-9]
DIGIT    [[:digit:]]
FLOAT    ({NONZERO}{DIGIT}*\.?|0\.|\.{DIGIT}){DIGIT}*|0
EXPONENT [eE](-|\+?){DIGIT}+

NUMBER   {FLOAT}{EXPONENT}?
WSPACE   [[:blank:]]+

%%

%{
   yylval = lval;
%}

{NUMBER} {
    yylval->emplace<int>(std::stoi(yytext));
    std::cout << "hit number" <<std::endl;
    return token::NUM;
};

{WSPACE} ;

"\n" {return token::NEWLINE;}

"/"  {return token::DIV; };
"*"  {return token::MUL; };
"%"  {return token::MOD; };
"+"  {return token::ADD; };
"-"  {return token::SUB; };
"^"  {return token::EXPONENT;};


"&" {return token::AND;};
"|" {return token::OR;};
"&&" {return token::BITAND;};
"||" {return token::BITOR;};

"!=" {return token::NEQ;};

"==" {return token::EQ;};

"!" {return token::NOT;};

. {
    fprintf(stderr,"lexical error.\n");
    exit(1);
}

%%