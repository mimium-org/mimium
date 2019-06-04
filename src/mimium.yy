%skeleton "lalr1.cc"
%require "3.4"

%defines
%define api.parser.class {MimiumParser}
%define api.namespace{mmmpsr}
%define api.value.type  variant

%{
#define YYDEBUG 1
#define YYERROR_VERBOSE 1


%}


%define api.token.prefix {TOK_}
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
;
%token <int> NUM "number"


%left  OR BITOR
%left  AND BITAND
%nonassoc  EQ NEQ
%left  ADD SUB
%left  MUL DIV MOD
%right NOT 

%%

expr : expr ADD expr  {$$ = node_op_new("+", $1, $3);}
     | expr SUB expr  {$$ = node_op_new("-", $1, $3);}
     | expr MUL expr  {$$ = node_op_new("*", $1, $3);}
     | expr DIV expr  {$$ = node_op_new("/", $1, $3);}
     | expr MOD expr  {$$ = node_op_new("%", $1, $3);}
     | expr EXPONENT expr  {$$ = node_op_new("^", $1, $3);}
     | expr OR expr  {$$ = node_op_new("|", $1, $3);}
     | expr AND expr  {$$ = node_op_new("&", $1, $3);}
     | expr BITOR expr  {$$ = node_op_new("||", $1, $3);}
     | expr BITAND expr  {$$ = node_op_new("&&", $1, $3);}
     | primary;

primary : NUM
        | '(' expr ')' {$$ =$2;};

%%

#include "lex.yy.c"

static void
yyerror(parser_state *p, const char *s)
{
  p->nerr++;
  if (p->fname) {
    fprintf(stderr, "%s:%d:%s\n", p->fname, p->lineno, s);
  }
  else {
    fprintf(stderr, "%d:%s\n", p->lineno, s);
  }
}