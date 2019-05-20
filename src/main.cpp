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
// #include "AST.hpp"


int main() {
    auto parser = pc::parser(fdef);
    std::string line = "1+2+2+1";

    while (std::getline(std::cin, line)) {
    auto res = parser.parse(line);
    if (res.is_success()) {
            std::cout << "Result = " << res.success().value()->to_string() << std::endl;
        }
        else {
            std::cout << "Failed to parse expression!" << std::endl;
        }
    }
    return 0;
}
