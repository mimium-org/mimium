#pragma once
#include <initializer_list>
#include <unordered_map>

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "type.hpp"
#include "mididriver.hpp"

namespace mimium {
class LLVMGenerator;
using builtintype = llvm::Value* (*)(std::vector<llvm::Value*>&, std::string,
                                     std::shared_ptr<LLVMGenerator>);
struct BuiltinFnInfo {
    BuiltinFnInfo()=default;
    BuiltinFnInfo(const BuiltinFnInfo& f) = default;
    BuiltinFnInfo(BuiltinFnInfo&& f)=default;
    ~BuiltinFnInfo()=default;
    BuiltinFnInfo&  operator=(const BuiltinFnInfo& b1)=default;
    BuiltinFnInfo&  operator=(BuiltinFnInfo&& b1)=default;

  BuiltinFnInfo(types::Function f,std::string s):mmmtype(std::move(f)),target_fnname(std::move(s)){}
  types::Value mmmtype;
  std::string target_fnname;
};
// using BuiltinFnInfo = std::pair<types::Function, std::string>;

struct LLVMBuiltin{
    static std::unordered_map<std::string, BuiltinFnInfo> ftable;
    static bool isBuiltin(std::string fname){ 
        return LLVMBuiltin::ftable.count(fname)>0;
    }

};
// class LLVMBuiltin {
//  public:
//   LLVMBuiltin();
//   ~LLVMBuiltin();
//   static llvm::Value* print(std::vector<llvm::Value*>& args, std::string name,
//                             std::shared_ptr<LLVMGenerator> generator);
// template<char* fname>
//   static llvm::Value* cmath(std::vector<llvm::Value*>& args, std::string name,
//                             std::shared_ptr<LLVMGenerator> generator) ;

//   const static bool isBuiltin(std::string str);
//   const static std::map<std::string, builtintype> builtin_fntable;
// };

}  // namespace mimium