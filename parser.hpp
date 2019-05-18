#include <iostream>
#include <string>
#include "CppCmb/cppcmb.hpp"

namespace pc = cppcmb;

namespace myparser{
template <char Ch>
bool is_same_char(char c) { return c == Ch; }

template <char Ch>
inline constexpr auto match = pc::one[pc::filter(is_same_char<Ch>)];


cppcmb_decl(expr_top,pc::product<>);
cppcmb_decl(expr, pc::product<>);
cppcmb_decl(assign, AST_Node);

cppcmb_decl(mul,      int);
cppcmb_decl(expon,    int);
cppcmb_decl(atom,     std::variant<std::string,int>);
cppcmb_decl(num,      int);
cppcmb_decl(symbol,      std::string);

cppcmb_decl(digit,    char);

cppcmb_def(expr_top) =
      expr & pc::end
    ;



cppcmb_def(assign)= 
    (symbol &match<'='> &expr);
int to_num(std::vector<char> const& chs) {
    int n = 0;
    for (auto c : chs) n = n * 10 + (c - '0');
    return n;
}
std::string to_string(std::vector<char> const& chs){
    std::string str(chs.begin(), chs.end());
    return str;
}
cppcmb_def(symbol) = (+pc::one[pc::filter(isalpha)])[to_string];


cppcmb_def(expr) = pc::pass
    | (expr & match<'+'> & mul) [do_op]
    | (expr & match<'-'> & mul) [do_op]
    | mul
    %= pc::as_memo_d;

cppcmb_def(mul) = pc::pass
    | (mul & match<'*'> & expon) [do_op]
    | (mul & match<'/'> & expon) [do_op]
    | expon
    %= pc::as_memo_d;

cppcmb_def(expon) = pc::pass
    | (atom & match<'^'> & expon) [do_op]
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

}

