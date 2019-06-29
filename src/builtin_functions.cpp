#include "builtin_functions.hpp"

// mValue mimium::builtin::cons(mValue val1,mValue val2){
//     return std::visit(overloaded{
//         [](double v1,AST_Ptr v2)->mValue{return std::make_pair(v1,v2);},
//         [](auto v1,auto v2)->mValue {return 0;}
//     },val1,val2);
// }
// mValue mimium::builtin::car(mValue val){
//     return std::visit(overloaded{
//         [](std::pair<double ,AST_Ptr> v)->mValue {return v.first;},
//         [](auto v)->mValue{return v;}
//     },val);
// }
// mValue mimium::builtin::cdr(mValue val){
//     return std::visit(overloaded{
//         [](std::pair<double ,AST_Ptr> v)->mValue {return v.second;},
//         [](auto v)->mValue{return 0;}
//     },val);
// }
mValue mimium::builtin::print(mValue str){
    std::visit(overloaded{
        [](double s){std::cout << s;},
        [](mClosure_ptr c){std::cout <<  c->to_string() ;},
        [](std::pair<double ,AST_Ptr> v){
            std::cout << "[ "<< v.first <<",";
            v.second->to_string(std::cout);
            std::cout <<" ]"; 
        },
        [](auto c){std::cout << "print function not implemented";}
    },str);
    return str;
}

mValue mimium::builtin::println(mValue str){
    mimium::builtin::print(str);
    std::cout << std::endl;
    return str;
}

bool mimium::builtin::isBuiltin(std::string str){
    return builtin_fntable.count(str)>0;
}