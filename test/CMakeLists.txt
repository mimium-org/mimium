cmake_minimum_required(VERSION 3.0)
project(mytest CXX)

find_package(GTest REQUIRED)

find_package(RtAudio REQUIRED)

BISON_TARGET(MyParser ../src/mimium.yy ${CMAKE_CURRENT_BINARY_DIR}/mimium_parser.cpp)
FLEX_TARGET(MyScanner ../src/mimium.l ${CMAKE_CURRENT_BINARY_DIR}/tokens.cpp)
ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)



add_executable(Test
${FLEX_MyScanner_OUTPUTS} ${BISON_MyParser_OUTPUTS} ../src/ast.cpp ../src/audiodriver.cpp  ../src/interpreter.cpp ../src/scheduler.cpp ../src/driver.cpp
 flex_bison_test.cpp  interpreter_test.cpp  builtin_test.cpp scheduler_test.cpp)


add_executable(AudioTest ../src/audiodriver.cpp audiodriver_test.cpp)

add_executable(SchedulerTest 
 ${FLEX_MyScanner_OUTPUTS} ${BISON_MyParser_OUTPUTS} ../src/ast.cpp ../src/audiodriver.cpp  ../src/interpreter.cpp ../src/scheduler.cpp ../src/driver.cpp 
scheduler_test.cpp)


target_include_directories(Test
    PRIVATE
    .
    ${CMAKE_CURRENT_BINARY_DIR}
    )
target_include_directories(SchedulerTest
    PRIVATE
    .
    ${CMAKE_CURRENT_BINARY_DIR}
  )
target_link_libraries(Test
  PRIVATE
    GTest::GTest
    GTest::Main   # GTest::Mainをリンクすると自分でmain関数を書く必要がなくなる
    rtaudio
    ${llvm_libs}
    builtin_functions
  )

target_link_libraries(AudioTest
  PRIVATE
  rtaudio
)
target_link_libraries(SchedulerTest
  PRIVATE
  rtaudio
  builtin_functions
)

file(GLOB testsource *.mmm)
file(COPY ${testsource} DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )