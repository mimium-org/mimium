
%{
  /* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
 
  #include <sstream>
  #include "compiler/scanner.hpp"
  #include "basic/helper_functions.hpp"
  #undef  YY_DECL
  #define YY_DECL int mmmpsr::MimiumScanner::yylex( MimiumParser::semantic_type * const lval, MimiumParser::location_type *loc )
  
  using token = MimiumParser::token;
  using namespace mimium;
  #define yyterminate() return  token::ENDFILE;
  #define YY_USER_ACTION loc->begin=loc->end; loc->end.col += yyleng;
  #define LOC_BREAK loc->end.line+=yyleng; loc->end.col=0;
%}
%option c++ noyywrap nounput noinput nodefault debug
%option yyclass="mmmpsr::MimiumParser"
%x comment
%x inlinecomment


NONZERO  [1-9]
DIGIT    [[:digit:]]
FLOAT    ({NONZERO}{DIGIT}*\.?|0\.|\.{DIGIT}){DIGIT}*|0
EXPONENT [eE](-|\+?){DIGIT}+
NUMBER   {FLOAT}{EXPONENT}?
SYMBOL   [[:alpha:]][[:alnum:]_]*
DELIMITER "{"|"}"|"("|")"|"["|"]"|","|"\""|"\."|"/"
WSPACE   [[:blank:]]+
STRING  "\""[[:alnum:][:graph:][:blank:]]+"\""
BREAK  "\r\n"|"\n"|"\r"
NEWLINE {BREAK}* | ";"
/*END {BREAK}{WSPACE}*"end"*/
/*{END} return token::END;*/
%%

%{
   yylval = lval;
%}

%{
  /* todo:need to make some token for bidirectional evaluation also for comment.*/
%}

"/*"		BEGIN(comment);
<comment>{
<<EOF>> { LexerError("unterminated comment detected.");}
[^*\n\r]*			/* eat anything that's not a '*' 		*/
"*"+[^*/\n\r]*		/* eat up '*'s not followed by '/'s 	*/
"*"+"/"{BREAK}+	{	BEGIN(INITIAL);
                  LOC_BREAK;
                  return token::NEWLINE;}
"*"+"/"				{BEGIN(INITIAL);}
{BREAK}+			{LOC_BREAK}
}

"//" BEGIN(inlinecomment);
<inlinecomment>{
<<EOF>> {return token::ENDFILE;}
[^\n\r]*
{BREAK}+ {LOC_BREAK;
          BEGIN(INITIAL);
          return token::NEWLINE;};
}

{NUMBER} {
    yylval->emplace<double>(std::stod(yytext));
    return token::NUM;
};

"now"   return token::NOW;


"float" return token::TYPEFLOAT;
"void" return token::TYPEVOID;
"string" return token::TYPESTRING;
"Fn" return token::TYPEFN;

":" return token::TYPE_DELIM;

"include" return token::INCLUDE;

"fn" return token::FUNC;
"self" { return token::SELF;}


"for" return token::FOR;
"in" return token::IN;

"return" return token::RETURN;
"if" return token::IF;
"else" return token::ELSE;

{STRING} {
  std::string str = yytext;
  //remove quotation
  str.erase(0,1);
  str.erase(str.end()-1,str.end());
  yylval->emplace<std::string>(str);
  return token::STRING;
};

{SYMBOL} {
  yylval->emplace<std::string>(yytext);
  return token::SYMBOL;
};

"->" return token::ARROW;
"|>" return token::PIPE;

"/"  return token::DIV; 
"*"  return token::MUL; 
"%"  return token::MOD; 
"+"  return token::ADD; 
"-"  return token::SUB; 
"^"  return token::EXPONENT;
"&" return token::AND;
"|" return token::OR;
"&&" return token::BITAND;
"||" return token::BITOR;
"!=" return token::NEQ;
"==" return token::EQ;
"!"  return token::NOT;
"<=" return token::LE;
">=" return token::GE;
"<<" return token::LSHIFT;
">>" return token::RSHIFT;
">" return token::GT;
"<" return token::LT;

"@" return token::AT;

"=" return token::ASSIGN;

"{" return token::LBRACE;
"}" return token::RBRACE;

{BREAK}+	{ LOC_BREAK return token::NEWLINE;}

{DELIMITER} { return yytext[0];};

{WSPACE} 

<<EOF>> {return token::ENDFILE;}

. {
  std::stringstream ss;
  ss << "Lexer: Unrecognized char \"" << yytext[0] << "\"\n";
    mimium::Logger::debug_log( ss.str() ,mimium::Logger::ERROR_);

}
%%

