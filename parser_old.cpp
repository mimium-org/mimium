#include<vector>
#include <iostream>
#include <string>
#include "CppCmb/cppcmb.hpp"

namespace pc = cppcmb;

template <char Ch>
bool is_same_char(char c) { return c == Ch; }

template <char Ch>
inline constexpr auto match = pc::one[pc::filter(is_same_char<Ch>)];

template <char Ch>
inline constexpr auto match_ =pc::skip(" ") & match<Ch>;


typedef struct _AST{
    std::string op;
    std::vector<pc::product<>> list;
} AST;


auto add_to_AST = [](auto op,auto list){
    AST res;
    res.op = op;
    res.list = list
    return res;
};

auto op_to_fcall = [](auto left, char op,auto right){
    std::vector<auto auto> list = {left,right};
    return fcall(op, list);
};

auto fcall = [](auto op,auto args){
    std::vector<std::string auto> list = {op,args}
    add_to_AST("fcall",list);
};

auto assign = [](auto symbol, auto expr){
    return add_to_AST("assign",expr);
};


cppcmb_decl(assign, pc::product<>);
cppcmb_decl(expr_top, pc::product<>);
cppcmb_decl(expr, pc::product<>);
cppcmb_decl(mul,      pc::product<>);
cppcmb_decl(expon,    pc::product<>);
cppcmb_decl(atom,     pc::product<>);
cppcmb_decl(num,      int);
cppcmb_decl(digit,    char);
cppcmb_decl(symbol, std::string);

int to_num(std::vector<char> const& chs) {
    int n = 0;
    for (auto c : chs) n = n * 10 + (c - '0');
    return n;
}

std::string to_string(std::vector<char> const& chs){
    std::string str(chs.begin(), chs.end());
    return str;
}



cppcmb_def(assign) =
    (symbol & match<'='>&expr & pc::end)[pc::select<0,2>] [assign];

cppcmb_def(expr_top) =
      expr & pc::end;

cppcmb_def(expr) = pc::pass
    | (expr & match<'+'> & mul) [op_to_fcall]
    | (expr & match<'-'> & mul) [op_to_fcall]
    | mul
    %= pc::as_memo_d;

cppcmb_def(mul) = pc::pass
    | (mul & match<'*'> & expon) [op_to_fcall]
    | (mul & match<'/'> & expon) [op_to_fcall]
    | expon
    %= pc::as_memo_d;

cppcmb_def(expon) = pc::pass
    | (atom & match<'^'> & expon) [op_to_fcall]
    | atom
    %= pc::as_memo_d;
cppcmb_def(atom) = pc::pass
    | (match<'('> & expr & match<')'>) [pc::select<1>]
    | symbol
    | num
    %= pc::as_memo_d;

cppcmb_def(num) =
      (+digit) [to_num]
    ;

cppcmb_def(digit) = pc::one[pc::filter(isdigit)];


cppcmb_def(symbol) = (+pc::one[pc::filter(isalpha)]) [to_string];


