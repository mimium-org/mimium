/**
 * A simple expression parser and evaluator. Evaluates user-input.
 * Supports +, -, *, /, ^ and grouping with parenthesis.
 * Correctly handles precedence associativity.
 */

#include <cctype>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include "parser.hpp"
#include "AST.hpp"


int main() {
    auto parser = pc::parser(myparser::assign);
    std::string line = "h=2+1+4";

        auto res = parser.parse(line).success().value();

           std::cout << "Result = " << res << std::endl;


    return 0;
}
