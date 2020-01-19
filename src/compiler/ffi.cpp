#include "compiler/ffi.hpp"
extern "C" {

void printdouble(double d) { std::cerr << d; }
void printlndouble(double d) { std::cerr << d << "\n"; }
double mimiumrand(){return ((double)rand()/RAND_MAX)*2 -1  ;}
}

namespace mimium {
using namespace types;
using FI = BuiltinFnInfo;
std::unordered_map<std::string, BuiltinFnInfo> LLVMBuiltin::ftable = {
    {"print", FI{Function(Void(), {Float()}), "printdouble"}},
    {"println", FI{Function(Void(), {Float()}), "printlndouble"}},

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

};

}  // namespace mimium