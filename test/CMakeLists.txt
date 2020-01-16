cmake_minimum_required(VERSION 3.0)
project(mytest CXX)

find_package(GTest REQUIRED)
include(GoogleTest)



BISON_TARGET(MyParser ../src/mimium.yy ${CMAKE_CURRENT_BINARY_DIR}/mimium_parser.cpp)
FLEX_TARGET(MyScanner ../src/mimium.l ${CMAKE_CURRENT_BINARY_DIR}/tokens.cpp)
ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)


add_executable(Test
${FLEX_MyScanner_OUTPUTS} ${BISON_MyParser_OUTPUTS} ../src/helper_functions.cpp ../src/ast.cpp ../src/runtime.cpp ../src/audiodriver.cpp  ../src/interpreter_visitor.cpp ../src/closure.cpp ../src/environment.cpp ../src/builtin_functions.cpp ../src/scheduler.cpp ../src/driver.cpp ../src/mididriver.cpp ../src/knormalize_visitor.cpp ../src/alphaconvert_visitor.cpp ../src/mir.cpp ../src/closure_convert.cpp ../src/type_infer_visitor.cpp ../src/llvmgenerator.cpp ../src/ffi.cpp
 flex_bison_test.cpp  interpreter_test.cpp  builtin_test.cpp knormalize_test.cpp alphaconv_test.cpp closureconvert_test.cpp typeinfer_test.cpp llvmgenerator_test.cpp)


add_executable(AudioTest ../src/audiodriver.cpp audiodriver_test.cpp)

add_executable(MidiTest 
${FLEX_MyScanner_OUTPUTS} ${BISON_MyParser_OUTPUTS} ../src/helper_functions.cpp ../src/ast.cpp ../src/audiodriver.cpp  ../src/closure.cpp ../src/environment.cpp ../src/builtin_functions.cpp ../src/runtime.cpp ../src/interpreter_visitor.cpp ../src/scheduler.cpp ../src/driver.cpp  ../src/mididriver.cpp rtmidi_test.cpp)


add_executable(SchedulerTest 
 ${FLEX_MyScanner_OUTPUTS} ${BISON_MyParser_OUTPUTS} ../src/helper_functions.cpp ../src/ast.cpp ../src/audiodriver.cpp  ../src/closure.cpp ../src/environment.cpp ../src/builtin_functions.cpp  ../src/runtime.cpp ../src/interpreter_visitor.cpp ../src/scheduler.cpp ../src/driver.cpp ../src/mididriver.cpp
scheduler_test.cpp)


target_include_directories(Test
    PRIVATE
    .
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GTEST_INCLUDE_DIRS}
    )
target_include_directories(SchedulerTest
    PRIVATE
    .
    ${CMAKE_CURRENT_BINARY_DIR}
  )
target_include_directories(MidiTest
  PRIVATE
  .
  ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(Test
  PRIVATE
    GTest::GTest
    GTest::Main   # GTest::Mainをリンクすると自分でmain関数を書く必要がなくなる
    ${SNDFILE_LIBRARIES}
    rtaudio
    rtmidi
    ${llvm_libs}
  )

target_link_libraries(AudioTest
  PRIVATE
  rtaudio
)
target_link_libraries(MidiTest
  PRIVATE
  rtaudio
  rtmidi
)
target_link_libraries(SchedulerTest
  PRIVATE
  rtaudio
  rtmidi
)

file(GLOB testsource *.mmm)
file(COPY ${testsource} DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )