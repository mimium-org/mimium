/**
 * A simple expression parser and evaluator. Evaluates user-input.
 * Supports +, -, *, /, ^ and grouping with parenthesis.
 * Correctly handles precedence associativity.
 */
#include <string>
#include <unistd.h>
#include "driver.hpp"

#include "interpreter.hpp"

    mmmpsr::MimiumDriver driver;
    mimium::Interpreter interpreter;
int main(int argc,char *argv[]) {
    interpreter.add_scheduler();
    if(argc==1){
    std::string line;
    std::cout << "start" <<std::endl;
    std::string teststr1 = "main = 1.245";
    
    while (std::getline(std::cin, line)) {
    driver.parsestring(line);
    mValue res = interpreter.loadAst(driver.getMainAst());
    std::string resstr = mimium::Interpreter::to_string(res);
    std::cout << resstr << std::endl;
    }
    }else{
        std::cout << argv[1]<<std::endl;
        driver.parsefile(argv[1]);
        interpreter.start();
        mValue res = interpreter.loadAst(driver.getMainAst());
        sleep(20);
        interpreter.stop();
    }
    return 0;
}
