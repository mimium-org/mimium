#include <unistd.h>

#include "driver.hpp"

#include "interpreter_visitor.hpp"
static mmmpsr::MimiumDriver driver;
static mimium::InterpreterVisitor interpreter;
int main() {
    auto runtime = interpreter.getRuntime();
    interpreter.init();
     driver.parsefile("test_time.mmm");
     runtime->start();
     sleep(20);
    runtime->stop();
    return 0;
}