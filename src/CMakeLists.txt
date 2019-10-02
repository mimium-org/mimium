cmake_minimum_required(VERSION 3.4)
PROJECT(MainCode CXX)

BISON_TARGET(MyParser mimium.yy ${CMAKE_CURRENT_BINARY_DIR}/mimium_parser.cpp)
FLEX_TARGET(MyScanner mimium.l ${CMAKE_CURRENT_BINARY_DIR}/tokens.cpp)
ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)

add_executable(Main ${FLEX_MyScanner_OUTPUTS} ${BISON_MyParser_OUTPUTS} main.cpp helper_functions.cpp ast.cpp environment.cpp closure.cpp builtin_functions.cpp interpreter.cpp interpreter_visitor.cpp driver.cpp audiodriver.cpp mididriver.cpp scheduler.cpp)

target_include_directories(Main
    PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    )
target_link_libraries(Main
    PRIVATE
    rtaudio
    rtmidi
      ${llvm_libs}
    )