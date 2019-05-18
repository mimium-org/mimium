#include <iostream>
#include <string>
#include "CppCmb/cppcmb.hpp"
#include "AST.hpp"
namespace pc = cppcmb;

template <char Ch>
bool is_same_char(char c) { return c == Ch; }

template <char Ch>
inline constexpr auto match = pc::one[pc::filter(is_same_char<Ch>)];


cppcmb_decl(expr_top,BaseAST);
cppcmb_decl(expr, BinaryExprAST);
cppcmb_decl(assign, AssignAST);

cppcmb_decl(mul,      BinaryExprAST);
cppcmb_decl(expon,    BinaryExprAST);
cppcmb_decl(atom,     BinaryExprAST);
cppcmb_decl(num,      NumberAST);
cppcmb_decl(symbol,      VariableAST);
cppcmb_decl(digit,    char);


NumberAST to_num(std::vector<char> const& chs) {
    int n = 0;
    for (auto c : chs) n = n * 10 + (c - '0');
    return NumberAST(n);
}

VariableAST to_variableast(std::vector<char> const& chs){
    const std::string str(chs.begin(), chs.end());
    return VariableAST(str);
}

AssignAST to_assign(std::string var,char eq,BaseAST expr){
    return AssignAST(var,expr);
}
cppcmb_def(assign)= 
    (symbol &match<'='> &expr)[to_assign];


BinaryExprAST to_exprast(NumberAST x,char op, NumberAST y){
    BinaryExprAST test =  BinaryExprAST(std::string({op}),x,y);
    return test;
}

cppcmb_def(expr_top) =
      expr & pc::end
    ;
cppcmb_def(expr) = pc::pass
    | (num & match<'+'> & num) [to_exprast]
    // | (expr & match<'-'> & mul) [to_exprast]
    // | atom
    %= pc::as_memo_d
    ;

cppcmb_def(mul) = pc::pass
    | (mul & match<'*'> & expon) [to_exprast]
    | (mul & match<'/'> & expon) [to_exprast]
    | expon
    %= pc::as_memo_d;

cppcmb_def(expon) = pc::pass
    | (atom & match<'^'> & expon) [to_exprast]
    | atom
    %= pc::as_memo_d;

cppcmb_def(atom) = pc::pass
    | (match<'('> & expr & match<')'>) [pc::select<1>]
    // | symbol
    | num
    %= pc::as_memo_d;

cppcmb_def(symbol) = (+pc::one[pc::filter(isalpha)])[to_variableast];

cppcmb_def(num) = (+digit) [to_num];

cppcmb_def(digit) = pc::one[pc::filter(isdigit)];



