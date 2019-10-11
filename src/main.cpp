/**
 * A simple expression parser and evaluator. Evaluates user-input.
 * Supports +, -, *, /, ^ and grouping with parenthesis.
 * Correctly handles precedence associativity.
 */
#include <string>
#include <unistd.h>
#include <csignal>
#include <fstream>
#include "helper_functions.hpp"
// #include "cli_tools.cpp"
#include "llvm/Support/CommandLine.h"
namespace cl = llvm::cl;
#include "driver.hpp"
#include "interpreter_visitor.hpp"


std::function<void(int)> shutdown_handler;
void signal_handler(int signo){
    shutdown_handler(signo);
}


int main(int argc,char** argv) {
    cl::opt<std::string> InputFilename(cl::Positional, cl::desc("<input file>"), cl::init("-"));
    cl::opt<bool> SndFile ("sndfile", cl::desc("write out a sound file as an output"));

    cl::ParseCommandLineOptions(argc, argv);//launch cli helper

    std::ifstream Input(InputFilename.c_str());
    signal(SIGINT,signal_handler);
    mimium::Logger::current_report_level = mimium::Logger::INFO;
    auto interpreter =  std::make_shared<mimium::InterpreterVisitor>();
    interpreter->init();
    auto& runtime = interpreter->getRuntime();
    shutdown_handler = [&runtime](int signal){
        if(runtime.isrunning()){
        runtime.stop();
        }
        std::cerr << std::endl << "Interuppted by key" << std::endl;
        exit(0);
    };

    runtime.add_scheduler(SndFile);
    if(!Input.good()){// filename is empty
    std::string line;
    std::cout << "start" <<std::endl;
    
    while (std::getline(std::cin, line)) {  
    runtime.clearDriver();  
    runtime.loadSource(line);
    // now load source is void function, how to debug print?
    // std::cout << resstr << std::endl;
    }
    }
    else{
        try{
        std::cout << InputFilename.c_str() <<std::endl;
        runtime.loadSourceFile(InputFilename.c_str());
        runtime.start();
        while(true){sleep(20);}; //todo : what is best way to wait infinitely? thread?
        }catch(std::exception& e){
        std::cerr << e.what()<<std::endl;
        runtime.stop();
        }
    }
    return 0;
}
