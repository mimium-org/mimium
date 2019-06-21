#include <unistd.h>

#include "driver.hpp"

#include "interpreter.hpp"
static mmmpsr::MimiumDriver driver;
static mimium::Interpreter interpreter;
int main() {
    interpreter.add_scheduler();
     driver.parsefile("test_time.mmm");
     interpreter.start();
     mValue res = interpreter.loadAst(driver.getMainAst());
     sleep(20);
    interpreter.stop();
    return 0;
}