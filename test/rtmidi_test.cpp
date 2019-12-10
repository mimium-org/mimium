#include <unistd.h>

#include "driver.hpp"
#include "interpreter_visitor.hpp"
static mmmpsr::MimiumDriver driver;
static std::shared_ptr<mimium::InterpreterVisitor> interpreter;
int main() {
    interpreter=std::make_shared<mimium::InterpreterVisitor>();
    auto runtime = interpreter->getRuntime();
   runtime->loadSourceFile("test_midi.mmm");
     runtime->start();
     sleep(30);
    runtime->stop();
    return 0;
}