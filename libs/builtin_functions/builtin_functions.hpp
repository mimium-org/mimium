#pragma once 

#include <string>
#include <map>
#include <iostream>
namespace mimium::builtin{

    void print(char ch);
    void print(std::string str);
    void println(std::string str);
    bool isBuiltin(std::string str);
    const static std::map<std::string,void(*)(std::string)> builtin_fntable = {
        {"printchar" ,&print},
        {"print", &print},
        {"println",&println}
    };
}