#include <iostream>
#include <string>
#include <tuple>
#include "cppcmb.hpp"
#include "s-exp.hpp"
namespace pc = cppcmb;

template <char Ch>
bool is_same_char(char c) { return c == Ch; }


template <char Ch>
inline constexpr auto match = pc::one[pc::filter(is_same_char<Ch>)];

inline constexpr auto wh =  +match<' '>;

template <char Ch>
inline constexpr auto matchwh =(-wh&match<Ch>&-wh)[pc::select<1>];

inline constexpr auto eol =  (-wh&(match<';'>|match<'\n'>| match<'\r'>) &-wh)[pc::select<1>];


using S_Ptr = std::shared_ptr<S_Expr>;


S_Ptr to_num(std::vector<char> const& chs) {
    const std::string str(chs.begin(), chs.end());
    S_Ptr res = std::make_shared<LiteralExpr>(str);
    // int n = 0;
    // for (auto c : chs) n = n * 10 + (c - '0');
    return res;
}

S_Ptr to_symbol(std::vector<char> const& chs){
    const std::string str(chs.begin(), chs.end());
    S_Ptr res = std::make_shared<LiteralExpr>(str);
    return res;
}
S_Ptr to_list(S_Ptr fst,pc::maybe<std::vector<S_Ptr>> rest){
    S_Ptr res = std::make_shared<ListExpr>();
   res->add_ptr(std::move(fst));
    if(rest.is_some()){
        for(auto &v :rest.some().value() )
        res->add_ptr(v);
    }
    return res;
}   
S_Ptr to_fcall(S_Ptr ident,S_Ptr args){
    S_Ptr res = std::make_shared<ListExpr>();
    res->add_str("fcall");
    res->add_ptr(ident);
    res->add_ptr(args);
    return res;
}
//this weird
S_Ptr to_fcall_raw(S_Ptr ident,char dm,S_Ptr args,char dm2){
    S_Ptr res = std::make_shared<ListExpr>();
    res->add_str("fcall");
    res->add_ptr(ident);
    res->add_ptr(args);
    return res;
}
S_Ptr binary_to_fcall(S_Ptr lhs,char op,S_Ptr rhs){
    S_Ptr args = std::make_shared<ListExpr>();
    args->add_ptr(lhs);
    args->add_ptr(rhs);
    S_Ptr ops = std::make_shared<LiteralExpr>(op);
    return  to_fcall(ops,args);
} 

S_Ptr to_lambda(S_Ptr args,S_Ptr body){
    S_Ptr res = std::make_shared<ListExpr>();
    res->add_str("lambda");
    res->add_ptr(args);
    res->add_ptr(body);
    return  res;
}
S_Ptr to_lambda_raw(char ch1,S_Ptr args,char ch2,char ch3,char ch4,char ch5,S_Ptr body,char ch6){
   return to_lambda(args,body);
}
S_Ptr to_assign (S_Ptr name,S_Ptr assignee){
    S_Ptr res = std::make_shared<ListExpr>();
    res->add_str("define");
    res->add_ptr(name);
    res->add_ptr(assignee);    
    return res;
};
S_Ptr to_assign_raw (S_Ptr name,char eq,S_Ptr assignee){
    return to_assign(name,assignee);
};
S_Ptr to_fdef (S_Ptr name,char lpar,S_Ptr args,char rpar,char eq,S_Ptr body){
    return to_assign(name,to_lambda(args,body));
};

S_Ptr to_top (std::vector<S_Ptr> sptrs){
    S_Ptr res = std::make_shared<ListExpr>(sptrs);
    return res;
};


S_Ptr to_array (char lbrc,S_Ptr sptrs,char rbrc){
    S_Ptr res = std::make_shared<ListExpr>();
    res->add_str("array");
    res->add_ptr(sptrs);
    return res;
};


cppcmb_decl(top,S_Ptr);
cppcmb_decl(statement,S_Ptr );


cppcmb_decl(expr_top,S_Ptr );
cppcmb_decl(expr, S_Ptr );
cppcmb_decl(fdef, S_Ptr );
cppcmb_decl(lambda, S_Ptr );

cppcmb_decl(arrayinit, S_Ptr );
cppcmb_decl(arrayrows, S_Ptr );
cppcmb_decl(arraycontent, S_Ptr );


cppcmb_decl(assign, S_Ptr );
cppcmb_decl(fcall, S_Ptr );
cppcmb_decl(list, S_Ptr );
cppcmb_decl(mul,      S_Ptr );
cppcmb_decl(expon,    S_Ptr );
cppcmb_decl(atom,     S_Ptr );
cppcmb_decl(num,     S_Ptr);
cppcmb_decl(symbol,      S_Ptr);
cppcmb_decl(digit,    char);

cppcmb_def(top)= (+((statement&eol)[pc::select<0>])&pc::end) [to_top];

cppcmb_def(statement)= pc::pass
    |(matchwh<'{'> &statement & matchwh<'}'>) [pc::select<1>]
    |assign
    |fdef 
;
cppcmb_def(fdef)= 
    (symbol&matchwh<'('>&list&matchwh<')'> & matchwh<'='> & expr) [to_fdef];

cppcmb_def(lambda)= 
    (matchwh<'('>&list&matchwh<')'>&match<'='>&match<'>'>&matchwh<'{'>&expr&matchwh<'}'> ) [to_lambda_raw]
    ;

cppcmb_def(assign)= 
    (symbol &matchwh<'='>& expr)[to_assign_raw];

cppcmb_def(arrayinit)=(matchwh<'['>&arrayrows&matchwh<']'>)[to_array];

cppcmb_def(arrayrows) = (arraycontent & -+(matchwh<';'> & arraycontent)[pc::select<1>]) [to_list];

cppcmb_def(arraycontent) = (num & -+((+wh[pc::select<>]) & (num) )[pc::select<1>])[to_list];

cppcmb_def(expr) = pc::pass
    | (expr & matchwh<'+'> &mul)[binary_to_fcall]
    | (expr & matchwh<'-'> & mul) [binary_to_fcall]
    | mul
    %= pc::as_memo_d
    ;

cppcmb_def(mul) = pc::pass
    | (mul & matchwh<'*'> & expon) [binary_to_fcall] 
    | (mul & matchwh<'/'> & expon) [binary_to_fcall]
    | expon
    %= pc::as_memo_d;

cppcmb_def(expon) = pc::pass
    | (atom & matchwh<'^'> & expon) [binary_to_fcall]
    | atom
    %= pc::as_memo_d;

cppcmb_def(atom) = pc::pass
    | (matchwh<'('> & expr & matchwh<')'>) [pc::select<1>]
    | fcall
    | symbol
    | num
    %= pc::as_memo_d;



cppcmb_def(list) = (symbol & -+((matchwh<','> & (atom) )[pc::select<1>]) )[to_list]%= pc::as_memo_d;;

//something is weird if we use pc::select<0,2> it fails
cppcmb_def(fcall) = (symbol& matchwh<'('>& list & matchwh<')'>) [to_fcall_raw];

cppcmb_def(symbol) = (+pc::one[pc::filter(isalpha)])[to_symbol];

cppcmb_def(num) = (+digit) [to_num]
                ;

cppcmb_def(digit) = pc::one[pc::filter(isdigit)];


