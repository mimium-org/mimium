#pragma once
#include <initializer_list>
#include <unordered_map>

#include "type.hpp"
#include "mididriver.hpp"

namespace mimium {

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

}  // namespace mimium