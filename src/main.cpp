/**
 * A simple expression parser and evaluator. Evaluates user-input.
 * Supports +, -, *, /, ^ and grouping with parenthesis.
 * Correctly handles precedence associativity.
 */
#include <string>
#include <unistd.h>
#include <csignal>
#include "helper_functions.hpp"
#include "driver.hpp"

#include "interpreter.hpp"


std::function<void(int)> shutdown_handler;
void signal_handler(int signo){
    shutdown_handler(signo);
}


int main(int argc,char** argv) {
    signal(SIGINT,signal_handler);
    mimium::Logger::current_report_level = mimium::Logger::WARNING;
    auto interpreter =  std::make_unique<mimium::Interpreter>();
    shutdown_handler = [&interpreter](int signal){
        if(interpreter->isrunning()){
        interpreter->stop();
        }
        std::cerr << std::endl << "Interuppted by key" << std::endl;
        exit(0);
    };

    interpreter->init();
    interpreter->add_scheduler();
    if(argc==1){
    std::string line;
    std::cout << "start" <<std::endl;
    
    while (std::getline(std::cin, line)) {  
    interpreter->clearDriver();  
    mValue res = interpreter->loadSource(line);
    std::string resstr = mimium::Interpreter::to_string(res);
    // std::cout << resstr << std::endl;
    }
    }
    else{
        try{
        std::cout << argv[1] <<std::endl;
        mValue res = interpreter->loadSourceFile(argv[1]);
        interpreter->start();
        while(true){sleep(20);}; //todo : what is best way to wait infinitely? thread?
        }catch(std::exception& e){
        std::cerr << e.what()<<std::endl;
        interpreter->stop();
        interpreter.reset();
        }
    }
    return 0;
}
