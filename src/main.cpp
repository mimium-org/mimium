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
    
    while (std::getline(std::cin, line)) {  
    interpreter.clearDriver();  
    mValue res = interpreter.loadSource(line);
    std::string resstr = mimium::Interpreter::to_string(res);
    std::cout << resstr << std::endl;
    }
    }
    else{
        std::cout << argv[1]<<std::endl;
        mValue res = interpreter.loadSourceFile(argv[1]);
        interpreter.start();
        sleep(20);
        interpreter.stop();
    }
    return 0;
}
