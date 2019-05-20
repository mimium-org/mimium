#include <iostream>
#include <string>
#include <tuple>
#include "CppCmb/cppcmb.hpp"
#include "s-exp.hpp"
namespace pc = cppcmb;

template <char Ch>
bool is_same_char(char c) { return c == Ch; }

template <char Ch>
inline constexpr auto match = pc::one[pc::filter(is_same_char<Ch>)];


cppcmb_decl(expr_top,std::shared_ptr<S_Expr> );
cppcmb_decl(expr, std::shared_ptr<S_Expr> );
cppcmb_decl(assign, std::shared_ptr<S_Expr> );

cppcmb_decl(mul,      std::shared_ptr<S_Expr> );
cppcmb_decl(expon,    std::shared_ptr<S_Expr> );
cppcmb_decl(atom,     std::shared_ptr<S_Expr> );
cppcmb_decl(num,     std::shared_ptr<S_Expr>);
cppcmb_decl(symbol,      std::shared_ptr<S_Expr>);
cppcmb_decl(digit,    char);


std::shared_ptr<S_Expr> to_num(std::vector<char> const& chs) {
    const std::string str(chs.begin(), chs.end());
    std::shared_ptr<S_Expr> res = std::make_shared<LiteralExpr>(str);
    // int n = 0;
    // for (auto c : chs) n = n * 10 + (c - '0');
    return res;
}

std::shared_ptr<S_Expr> to_symbol(std::vector<char> const& chs){
    const std::string str(chs.begin(), chs.end());
    std::shared_ptr<S_Expr> res = std::make_shared<LiteralExpr>(str);
    return res;
}

// AssignAST to_assign(std::string var,char eq,BaseAST expr){
//     return AssignAST(var,expr);
// }


auto to_assign = [](std::string name,char eq,auto assignee){
    return std::make_tuple("assign",name,assignee);
};


std::shared_ptr<S_Expr> binary_to_fcall(std::shared_ptr<S_Expr> lhs,char op,std::shared_ptr<S_Expr> rhs){
    std::shared_ptr<S_Expr> args = std::make_shared<ListExpr>();
    args->add_ptr(lhs);
    args->add_ptr(rhs);
    std::shared_ptr<S_Expr> res = std::make_shared<ListExpr>();
    res->add_str("fcall");
    res->add_str(op);
    res->add_ptr(args);
    return res;
} 

auto lambda = [](std::vector<std::string> args,auto body){
    return std::make_tuple("lambda",args,body);
};
auto fdef = [](std::string name,std::vector<std::string> args,auto body){
    return assign(name,lambda(args,body));
};

cppcmb_def(assign)= 
    (symbol &match<'='>& expr)[to_assign];

cppcmb_def(expr_top) =
      expr & pc::end [pc::select<0>]
    ;

cppcmb_def(expr) = pc::pass
    | (expr & match<'+'> &mul)[binary_to_fcall]
    | (expr & match<'-'> & mul) [binary_to_fcall]
    | mul
    %= pc::as_memo_d
    ;

cppcmb_def(mul) = pc::pass
    | (mul & match<'*'> & expon) [binary_to_fcall] 
    | (mul & match<'/'> & expon) [binary_to_fcall]
    | expon
    %= pc::as_memo_d;

cppcmb_def(expon) = pc::pass
    | (atom & match<'^'> & expon) [binary_to_fcall]
    | atom
    %= pc::as_memo_d;

cppcmb_def(atom) = pc::pass
    | (match<'('> & expr & match<')'>) [pc::select<1>]
    | symbol
    | num
    %= pc::as_memo_d;

cppcmb_def(symbol) = (+pc::one[pc::filter(isalpha)])[to_symbol];

    cppcmb_def(num) = (+digit) [to_num]
                ;

cppcmb_def(digit) = pc::one[pc::filter(isdigit)];



