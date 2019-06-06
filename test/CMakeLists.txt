cmake_minimum_required(VERSION 3.0)
project(Testcode CXX)


find_package(GTest REQUIRED)
list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/flex)
list(APPEND CMAKE_PREFIX_PATH /usr/local/opt/bison)
find_package(BISON 3.2 REQUIRED)
find_package(Flex REQUIRED)
include_directories(../src ${FLEX_INCLUDE_DIRS})


BISON_TARGET(MyParser ../src/mimium.yy ${CMAKE_CURRENT_BINARY_DIR}/mimium_parser.cpp)
FLEX_TARGET(MyScanner ../src/mimium.l ${CMAKE_CURRENT_BINARY_DIR}/tokens.cpp)
ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)



add_executable(mytest 
${FLEX_MyScanner_OUTPUTS} ${BISON_MyParser_OUTPUTS} ../src/s-exp.cpp ../src/interpreter.cpp ../src/driver.cpp
 flex_bison_test.cpp  )
target_include_directories(mytest
    PRIVATE
    .
    ${CMAKE_CURRENT_BINARY_DIR}
    )
target_link_libraries(mytest
  PRIVATE
    GTest::GTest
    GTest::Main   # GTest::Mainをリンクすると自分でmain関数を書く必要がなくなる
  )