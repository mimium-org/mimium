/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
 

#include "compiler/ffi.hpp"
extern "C" {
void dumpaddress(void* a) { std::cerr << a <<"\n"; }

void printdouble(double d) { std::cerr << d; }
void printlndouble(double d) { std::cerr << d << "\n"; }

void printlnstr(char* str){ std::cerr << str << "\n"; }

double mimiumrand(){return ((double)rand()/RAND_MAX)*2 -1  ;}

double mimium_ifexpr(double cond,double thenval,double elseval){
    return (cond>0)?thenval:elseval;
}
bool mimium_dtob(double d){
    return d>0;
}
int64_t mimium_dtoi(double d){
    return static_cast<int64_t>(d);
}
double mimium_gt(double d1,double d2){
    return static_cast<double>(d1>d2);
}
double mimium_lt(double d1,double d2){
    return static_cast<double>(d1<d2);
}
double mimium_ge(double d1,double d2){
    return static_cast<double>(d1>=d2);
}
double mimium_le(double d1,double d2){
    return static_cast<double>(d1<=d2);
}
double mimium_and(double d1,double d2){
    return static_cast<double>(mimium_dtob(d1)&&mimium_dtob(d2));
}
double mimium_or(double d1,double d2){
    return static_cast<double>(mimium_dtob(d1)||mimium_dtob(d2));
}
double mimium_lshift(double d1,double d2){
    return static_cast<double>(mimium_dtoi(d1)<<mimium_dtoi(d2));
}
double mimium_rshift(double d1,double d2){
    return static_cast<double>(mimium_dtoi(d1)>>mimium_dtoi(d2));
}

double mimium_memprim(double d,double* mem){
    auto tmp = *mem;
    *mem = d;
    return tmp;
}

}

namespace mimium {
using namespace types;
using FI = BuiltinFnInfo;
std::unordered_map<std::string, BuiltinFnInfo> LLVMBuiltin::ftable = {
    {"print", FI{Function(Void(), {Float()}), "printdouble"}},
    {"println", FI{Function(Void(), {Float()}), "printlndouble"}},
    {"printlnstr", FI{Function(Void(), {String()}), "printlnstr"}},



    {"sin", FI{Function(Float(), {Float()}), "sin"}},
    {"cos", FI{Function(Float(), {Float()}), "cos"}},
    {"tan", FI{Function(Float(), {Float()}), "tan"}},

    {"asin", FI{Function(Float(), {Float()}), "asin"}},
    {"acos", FI{Function(Float(), {Float()}), "acos"}},
    {"atan", FI{Function(Float(), {Float()}), "atan"}},
    {"atan2", FI{Function(Float(), {Float(), Float()}), "atan2"}},

    {"sinh", FI{Function(Float(), {Float()}), "sinh"}},
    {"cosh", FI{Function(Float(), {Float()}), "cosh"}},
    {"tanh", FI{Function(Float(), {Float()}), "tanh"}},

    {"exp", FI{Function(Float(), {Float()}), "exp"}},
    {"pow", FI{Function(Float(), {Float(),Float()}), "pow"}},

    {"log", FI{Function(Float(), {Float()}), "log"}},
    {"log10", FI{Function(Float(), {Float()}), "log10"}},
    {"random", FI{Function(Float(), {}), "mimiumrand"}},

    {"sqrt", FI{Function(Float(), {Float()}), "sqrt"}},
    {"abs", FI{Function(Float(), {Float()}), "fabs"}},

    {"ceil", FI{Function(Float(), {Float()}), "ceil"}},
    {"floor", FI{Function(Float(), {Float()}), "floor"}},
    {"trunc", FI{Function(Float(), {Float()}), "trunc"}},
    {"round", FI{Function(Float(), {Float()}), "round"}},

    {"fmod", FI{Function(Float(), {Float(),Float()}), "fmod"}},
    {"remainder", FI{Function(Float(), {Float(),Float()}), "remainder"}},

    {"min", FI{Function(Float(), {Float(),Float()}), "fmin"}},
    {"max", FI{Function(Float(), {Float(),Float()}), "fmax"}},


    {"ge", FI{Function(Float(), {Float(),Float()}), "mimium_ge"}},
    {"le", FI{Function(Float(), {Float(),Float()}), "mimium_le"}},
    {"gt", FI{Function(Float(), {Float(),Float()}), "mimium_gt"}},
    {"lt", FI{Function(Float(), {Float(),Float()}), "mimium_lt"}},
    {"and", FI{Function(Float(), {Float(),Float()}), "mimium_and"}},
    {"or", FI{Function(Float(), {Float(),Float()}), "mimium_or"}},
    {"lshift", FI{Function(Float(), {Float(),Float()}), "mimium_lshift"}},
    {"rshift", FI{Function(Float(), {Float(),Float()}), "mimium_rshift"}},
    {"ifexpr", FI{Function(Float(), {Float(),Float(),Float()}), "mimium_ifexpr"}},

    {"mem", FI{Function(Float(), {Float()}), "mimium_memprim"}}

};

}  // namespace mimium