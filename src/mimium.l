%{
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>


  extern "C" int yywrap( void ){ return 1 ; }
%}

NONZERO  [1-9]
DIGIT    [[:digit:]]
FLOAT    ({NONZERO}{DIGIT}*\.?|0\.|\.{DIGIT}){DIGIT}*|0
EXPONENT [eE](-|\+?){DIGIT}+

NUMBER   {FLOAT}{EXPONENT}?
WSPACE   [[:blank:]]+

%%

NUMBER {
    double temp;
    sscanf(yytext,"%lf",&temp);
    yylval.double_value = temp;
    return NUM
}

WSPACE ;



"*"  return MUL ;
"/"  return DIV ;
"%"  return MOD ;
"+"  return ADD ;
"-"  return SUB ;
"^"  return EXPONENT;


"&" return AND;
"|" return OR;
"&&" return BITAND;
"||" return BITOR;

"!=" return NEQ;

"==" return EQ;

"!" return NOT;

. {
    fprintf(stderr,"lexical error.\n");
    exit(1);
}

%%