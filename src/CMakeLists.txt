cmake_minimum_required(VERSION 3.4)
PROJECT(MainCode CXX)

BISON_TARGET(MyParser mimium.yy ${CMAKE_CURRENT_BINARY_DIR}/mimium_parser.cpp)
FLEX_TARGET(MyScanner mimium.l ${CMAKE_CURRENT_BINARY_DIR}/tokens.cpp)
ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)

add_executable(Main ${FLEX_MyScanner_OUTPUTS} ${BISON_MyParser_OUTPUTS} main.cpp helper_functions.cpp ast.cpp environment.cpp runtime.cpp  driver.cpp audiodriver.cpp mididriver.cpp scheduler.cpp mir.cpp alphaconvert_visitor.cpp knormalize_visitor.cpp type_infer_visitor.cpp closure_convert.cpp llvmgenerator.cpp llvm_builtin_functions.cpp)

target_include_directories(Main
    PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    )
target_link_libraries(Main
    PRIVATE
    rtaudio
    rtmidi
    ${SNDFILE_LIBRARIES}
      ${llvm_libs}
    )