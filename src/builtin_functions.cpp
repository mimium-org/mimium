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
mValue mimium::builtin::print(std::shared_ptr<ArgumentsAST> argast,mimium::Interpreter* interpreter){
    auto args = argast->getElements();
    for (auto& elem : args){
    mValue ev = interpreter->interpretExpr(elem);
    std::cout << Interpreter::to_string(ev);
    std::cout <<" ";
    }
    return 0.0;
}

mValue mimium::builtin::println(std::shared_ptr<ArgumentsAST> argast,mimium::Interpreter* interpreter){
    mimium::builtin::print(argast,interpreter);
    std::cout << std::endl;
    return 0.0;
}

bool mimium::builtin::isBuiltin(std::string str){
    return builtin_fntable.count(str)>0;
}