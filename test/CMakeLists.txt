cmake_minimum_required(VERSION 3.0)
project(Testcode CXX)

find_package(GTest REQUIRED)

include_directories(../src)

add_executable(mytest 
../src/s-exp.cpp 
parser_test.cpp)
target_link_libraries(mytest
  PRIVATE
    GTest::GTest
    GTest::Main   # GTest::Mainをリンクすると自分でmain関数を書く必要がなくなる
  )