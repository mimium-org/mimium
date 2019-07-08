#include <unistd.h>

#include "driver.hpp"
#include "interpreter.hpp"
static mmmpsr::MimiumDriver driver;
static mimium::Interpreter interpreter;
int main() {
    interpreter.add_scheduler();
     driver.parsefile("test_midi.mmm");
     interpreter.start();
     mValue res = interpreter.loadAst(driver.getMainAst());
     sleep(30);
    interpreter.stop();
    return 0;
}