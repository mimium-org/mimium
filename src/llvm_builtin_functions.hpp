#pragma once
#include <initializer_list>
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


#include "mididriver.hpp"
#include "llvmgenerator.hpp"

namespace mimium {
    class LLVMGenerator;
    using builtintype = llvm::Value* (*)(std::vector<llvm::Value*>&,std::string,std::shared_ptr<LLVMGenerator>);
    struct BulitinFnInfo{
        // BuiltinFnInfo(std::initializer_list<Buil>)
        std::string name;
        types::Function mmmtype;
        builtintype fn_ptr;    
    };

    class LLVMBuiltin{

        public:
        LLVMBuiltin();
        ~LLVMBuiltin();
        static llvm::Value* print(std::vector<llvm::Value*>& args,std::string name,std::shared_ptr<LLVMGenerator> generator);
        const static bool isBuiltin(std::string str);
        const static std::map<std::string,builtintype> builtin_fntable; 
    };
    
}  // namespace mimium