#pragma once

#include <unordered_map>
#include <string>
#include "type.hpp"

namespace mimium::builtin {
    const static std::unordered_map<std::string,types::Value> types_map={
        {"print",types::Function(types::Function::createArgs(types::Float()),types::Void())},
         {"sin",types::Function(types::Function::createArgs(types::Float()),types::Float())}
    };
    
    }