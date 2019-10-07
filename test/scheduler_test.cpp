#include <unistd.h>

#include "driver.hpp"

#include "interpreter_visitor.hpp"
static mmmpsr::MimiumDriver driver;
static mimium::InterpreterVisitor interpreter;
int main() {
    interpreter.init();
    interpreter.add_scheduler();
     driver.parsefile("test_time.mmm");
     interpreter.start();
     mValue res = interpreter.loadAst(driver.getMainAst());
     sleep(20);
    interpreter.stop();
    return 0;
}