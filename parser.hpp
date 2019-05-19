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


cppcmb_decl(expr_top,S_Expr*);
cppcmb_decl(expr, S_Expr*);
cppcmb_decl(assign, S_Expr*);

cppcmb_decl(mul,      S_Expr*);
cppcmb_decl(expon,    S_Expr*);
cppcmb_decl(atom,     S_Expr*);
cppcmb_decl(num,     S_Expr*);
cppcmb_decl(symbol,      S_Expr*);
cppcmb_decl(digit,    char);


S_Expr* to_num(std::vector<char> const& chs) {
    const std::string str(chs.begin(), chs.end());
    LiteralExpr* res = new LiteralExpr(str);
    // int n = 0;
    // for (auto c : chs) n = n * 10 + (c - '0');
    return res;
}

S_Expr* to_symbol(std::vector<char> const& chs){
    const std::string str(chs.begin(), chs.end());
    LiteralExpr* res = new LiteralExpr(str);
    return res;
}

// AssignAST to_assign(std::string var,char eq,BaseAST expr){
//     return AssignAST(var,expr);
// }


// BinaryExprAST to_exprast(NumberAST x,char op, NumberAST y){
//     BinaryExprAST test =  BinaryExprAST(std::string({op}),x,y);
//     return test;
// }

auto to_assign = [](std::string name,char eq,auto assignee){
    return std::make_tuple("assign",name,assignee);
};


std::tuple<std::string,std::string,std::vector<std::string>> to_fcall(std::string opname,std::vector<std::string> args){
    return std::make_tuple("fcall",opname,args);
};
auto to_fcall(char opname,std::vector<std::string> args){
    std::string opstring({&opname});
    return to_fcall(opstring,args);
};
// auto binary_to_fcall(std::string x,char op,std::string y){
//     return to_fcall(op,std::vector<std::string>({x,y}));
// };
S_Expr* binary_to_fcall(S_Expr* lhs,char op,S_Expr* rhs){
    ListExpr* args = new ListExpr();
    args->exprs.push_back(lhs);
    args->exprs.push_back(rhs);
    ListExpr* res = new ListExpr();
    LiteralExpr* fcall = new LiteralExpr("fcall");
    LiteralExpr* ops = new LiteralExpr(op);
    res->exprs.push_back(fcall);
    res->exprs.push_back(ops);
    res->exprs.push_back(args);
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
      expr & pc::end
    ;

cppcmb_def(expr) = pc::pass
    | (expr & match<'+'> &num)[binary_to_fcall]
    | (expr & match<'-'> & mul) [binary_to_fcall]
    | num 
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



