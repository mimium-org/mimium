#include "llvm_builtin_functions.hpp"

#include "llvm/IR/DerivedTypes.h"

extern "C" {

void printdouble(double d) { std::cerr << d << "\n"; }
}

namespace mimium {
using  namespace std::string_literals;
using Fn = types::Function;
std::unordered_map<std::string, BuiltinFnInfo> LLVMBuiltin::ftable = {
    {"print",
     BuiltinFnInfo{Fn::create(types::Void(), {types::Float()}), "printdouble"}},
     {"sin",
      BuiltinFnInfo{Fn::create(types::Float(), {types::Float()}), "sin"}}
      };

}  // namespace mimium