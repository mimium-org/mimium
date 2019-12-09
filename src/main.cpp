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
void signalHandler(int signo){
    shutdown_handler(signo);
}


auto main(int argc,char** argv) -> int {
    cl::opt<std::string> input_filename(cl::Positional, cl::desc("<input file>"), cl::init("-"));
    cl::opt<bool> snd_file ("sndfile", cl::desc("write out a sound file as an output"));

    cl::ParseCommandLineOptions(argc, argv);//launch cli helper

    std::ifstream input(input_filename.c_str());
    signal(SIGINT,signalHandler);
    mimium::Logger::current_report_level = mimium::Logger::INFO;
    auto interpreter =  std::make_shared<mimium::InterpreterVisitor>();
    interpreter->init();
    auto runtime = interpreter->getRuntime();
    shutdown_handler = [&runtime](int /*signal*/){
        if(runtime->isrunning()){
        runtime->stop();
        }
        std::cerr << std::endl << "Interuppted by key" << std::endl;
        exit(0);
    };

    runtime->add_scheduler(snd_file);
    if(!input.good()){// filename is empty
    std::string line;
    std::cout << "start" <<std::endl;
    
    while (std::getline(std::cin, line)) {  
    runtime->clearDriver();  
    runtime->loadSource(line);
    // now load source is void function, how to debug print?
    // std::cout << resstr << std::endl;
    }
    }
    else{
        try{
        std::cout << input_filename.c_str() <<std::endl;
        runtime->loadSourceFile(input_filename.c_str());
        runtime->start();
        while(true){sleep(20);}; //todo : what is best way to wait infinitely? thread?
        }catch(std::exception& e){
        std::cerr << e.what()<<std::endl;
        runtime->stop();
        }
    }
    return 0;
}
