/**
 * A simple expression parser and evaluator. Evaluates user-input.
 * Supports +, -, *, /, ^ and grouping with parenthesis.
 * Correctly handles precedence associativity.
 */
#include <string>
#include "driver.hpp"

#include "interpreter.hpp"
    mmmpsr::MimiumDriver driver;
    mimium::Interpreter interpreter;

int main() {
    std::string line;
    std::cout << "start" <<std::endl;
    std::string teststr1 = "main = 1.245";

    while (std::getline(std::cin, line)) {
    driver.parsestring(line);
    interpreter.loadAst(driver.getMainAst());
    double res = interpreter.findVariable("main");
    std::cout << res << std::endl;
    }
    return 0;
}
