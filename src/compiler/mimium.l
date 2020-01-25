%{
  #include <sstream>
  #include "compiler/scanner.hpp"
  #include "basic/helper_functions.hpp"
  #undef  YY_DECL
  #define YY_DECL int mmmpsr::MimiumScanner::yylex( mmmpsr::MimiumParser::semantic_type * const lval, mmmpsr::MimiumParser::location_type *loc )
  
  using token = mmmpsr::MimiumParser::token;
  using namespace mimium;
  #define yyterminate() return ( token::ENDFILE )
  #define YY_USER_ACTION loc->step(); loc->columns(yyleng);
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
STRING  "\""[[:alnum:][:graph:]]+"\""
DELIMITER "{"|"}"|"("|")"|"["|"]"|","|"\""|"\."|"/"
WSPACE   [[:blank:]]+
BREAK  "\r\n"|"\n"|"\r"
NEWLINE {BREAK}* | ";"
/*END {BREAK}{WSPACE}*"end"*/
/*{END} return token::END;*/
%%

%{
   yylval = lval;
%}


"/*"		BEGIN(comment);
<comment>{
[^*\n\r]*			/* eat anything that's not a '*' 		*/
"*"+[^*/\n\r]*		/* eat up '*'s not followed by '/'s 	*/
{BREAK}+			{loc->lines(yyleng);}
"*"+"/"{BREAK}*				{BEGIN(INITIAL);}
}

"//" BEGIN(inlinecomment);
<inlinecomment>{
{BREAK}+ {BEGIN(INITIAL); loc->lines(yyleng);};
[^\n\r]*
}

{NUMBER} {
    yylval->emplace<double>(std::stod(yytext));
    return token::NUM;
};

"float" return token::TYPEFLOAT;
"void" return token::TYPEVOID;
"Fn" return token::TYPEFN;

":" return token::TYPE_DELIM;

"include" return token::INCLUDE;

"fn" return token::FUNC;
"self" return token::SELF;


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
"<<" return token::LSHIFT;
">>" return token::RSHIFT;
">" return token::GT;
"<" return token::LT;

"@" return token::AT;

"=" return token::ASSIGN;

"{" return token::LBRACE;
"}" return token::RBRACE;

{BREAK}+	{ loc->lines(yyleng); return token::NEWLINE;}

{DELIMITER} { return yytext[0];};

{WSPACE} 

. {
  std::stringstream ss;
  ss << "Lexer: Unrecognized char \"" << yytext[0] << "\"\n";
    mimium::Logger::debug_log( ss.str() ,mimium::Logger::ERROR);
		loc->step();
}
%%

