#include "helper_functions.hpp"
#include "driver.hpp"
#include "gtest/gtest.h"

static mmmpsr::MimiumDriver driver;

TEST(bison_parser_test, assign) {
  driver.setWorkingDirectory("/Users/tomoya/codes/mimium/build/test/");
  std::string teststr = "a = 1";
  driver.parsestring(teststr);
  std::stringstream ss;
  driver.print(ss);
  EXPECT_EQ(ss.str(), "(assign a 1)");
}
TEST(bison_parser_test, assignfpoint) {
  driver.clear();
  std::string teststr = "a = 1.245";
  driver.parsestring(teststr);
  std::stringstream ss;
  driver.print(ss);
  EXPECT_EQ(ss.str(), "(assign a 1.245)");
}

TEST(bison_parser_test, expr) {
  driver.clear();
  std::string teststr = "a = 1+2+3*2/2";
  driver.parsestring(teststr);
  std::stringstream ss;
  driver.print(ss);
  EXPECT_EQ(ss.str(), "(assign a (+ (+ 1 2) (/ (* 3 2) 2)))");
}

TEST(bison_parser_test, parensis) {
  driver.clear();
  std::string teststr = "a=(2+3)/5";
  driver.parsestring(teststr);
  std::stringstream ss;
  driver.print(ss);
  EXPECT_EQ(ss.str(), "(assign a (/ (+ 2 3) 5))");
}

TEST(bison_parser_test, lines) {
  driver.clear();
  std::string teststr = "a=2 \n b=3";
  driver.parsestring(teststr);
  std::stringstream ss;
  driver.print(ss);
  EXPECT_EQ(ss.str(), "((assign a 2) (assign b 3))");
}

TEST(bison_parser_test, fdef) {
  driver.clear();
  std::string teststr = "fn myfunc(a,b){return 1+2}";
  driver.parsestring(teststr);
  std::stringstream ss;
  driver.print(ss);
  EXPECT_EQ(ss.str(), "(assign myfunc (lambda (a b) (return (+ 1 2))))");
}
TEST(bison_parser_test, fdef_multi) {
  driver.clear();
  std::string teststr =
      "fn myfunc(a,b){\n return 1+2 \n} \n fn myfuncb(c,d){\nreturn 1+2\n}";
  driver.parsestring(teststr);
  std::stringstream ss;
  driver.print(ss);
  EXPECT_EQ(ss.str(),
            "((assign myfunc (lambda (a b) (return (+ 1 2)))) (assign myfuncb "
            "(lambda (c d) (return (+ 1 2)))))");
}
TEST(bison_parser_test, fdef_multi2) {
  driver.clear();
  std::string teststr =
      "fn makecounter(x){\n\
          localvar = 0\n\
          fn countup(y){\n\
               localvar= localvar+x\n\
               return localvar\n\
          }\n\
          return countup\n\
     }\n\
     ctr = makecounter(1)\n\
     fn rec(y){\n\
          myctr = ctr(1)\n\
          test = println(myctr)@x\n\
          return rec(x)\n\
     }\n\
     main = rec(48000)@0";
  driver.parsestring(teststr);
  std::stringstream ss;
  driver.print(ss);
  EXPECT_EQ(
      ss.str(),
      "((assign makecounter (lambda (x) ((assign localvar 0) (assign countup "
      "(lambda (y) ((assign localvar (+ localvar x)) (return localvar)))) "
      "(return countup)))) (assign ctr (makecounter (1))) (assign rec (lambda "
      "(y) ((assign myctr (ctr (1))) (assign test (println (myctr))@x) (return "
      "(rec (x)))))) (assign main (rec (48000))@0))");
}

TEST(bison_parser_test, lambda) {
  driver.clear();

  std::string teststr = "myfunc = (a,b)->{return 1+2}";
  driver.parsestring(teststr);
  std::stringstream ss;
  driver.print(ss);
  EXPECT_EQ(ss.str(), "(assign myfunc (lambda (a b) (return (+ 1 2))))");
}

TEST(bison_parser_test, time) {
  driver.clear();
  std::string teststr = "a = (3+2)@50";
  driver.parsestring(teststr);
  std::stringstream ss;
  driver.print(ss);
  EXPECT_EQ(ss.str(), "(assign a (+ 3 2)@50)");
}

TEST(bison_parser_test, fileread) {
  driver.clear();
  driver.parsefile("testfile1.mmm");
  std::stringstream ss;
  driver.print(ss);
  EXPECT_EQ(ss.str(), "((assign main (+ 2 3)) (assign main2 (1 2 3)) (assign main3 arrayaccess main2 1))");
}

TEST(bison_parser_test, func_block) {
  driver.clear();
  driver.parsefile("testfile_blockfun.mmm");
  std::stringstream ss;
  driver.print(ss);
  EXPECT_EQ(ss.str(), "(assign hoge (lambda (x y) (return 1)))");
}

TEST(bison_parser_test, statements) {
  driver.clear();
  driver.parsefile("testfile_statements.mmm");
  std::stringstream ss;
  driver.print(ss);
  EXPECT_EQ(ss.str(),
            "((assign hoge (lambda (x y) (return (+ x y)))) (assign main (hoge (3 5))))");
}
TEST(bison_parser_test, nestfunction) {
  driver.clear();
  driver.parsefile("test_localvar2.mmm");
  std::stringstream ss;
  driver.print(ss);
  EXPECT_EQ(
      ss.str(),
      "((assign hoge (lambda (x) ((assign localvar x) (assign fuga (lambda (y) ((assign localvar (+ localvar y)) (return localvar)))) (return (fuga (2.5)))))) (assign main (hoge (1))))");
}

TEST(bison_parser_test, ifstatement) {
  driver.clear();

  driver.parsefile("test_if.mmm");
  std::stringstream ss;
  driver.print(ss);
  std::cout << ss.str() << std::endl;
  EXPECT_EQ(ss.str(),
            "((assign test (lambda (x y) ((if x (assign res y) (assign res 100)) (return res)))) (assign true (test (1 5))) (assign false (test (0 100))))");
}
TEST(bison_parser_test, ifstatement2) {
  driver.clear();

  driver.parsefile("test_if_nested.mmm");
  std::stringstream ss;
  driver.print(ss);
  std::cout << ss.str() << std::endl;
  EXPECT_EQ(ss.str(),
            "((assign test (lambda (x y z) ((if x (assign res 0) (if y (assign res 100) (assign res z))) (return res)))) (assign zero (test (1 23 244))) (assign hand (test (0 200 400))) (assign fivehand (test (0 0 500))))");
}
TEST(bison_parser_test, array) {
  driver.clear();

  std::string teststr = "main = [1,2,3,4,5]";
  driver.parsestring(teststr);
  std::stringstream ss;
  driver.print(ss);
  EXPECT_EQ(ss.str(), "(assign main (1 2 3 4 5))");
}
TEST(bison_parser_test, arrayaccess) {
  driver.clear();

  std::string teststr =
      "arr = [1,2,3,4,5]\n\
                         main = arr[2]";
  driver.parsestring(teststr);
  std::stringstream ss;
  driver.print(ss);
  EXPECT_EQ(ss.str(),
            "((assign arr (1 2 3 4 5)) (assign main arrayaccess arr 2))");
}
TEST(bison_parser_test, forloop) {
  driver.clear();

  std::string teststr =
      "arr = [1,2,3,4,5]\n\
                         for i in arr {\n\
                              main = i  \n\
                         }";
  driver.parsestring(teststr);
  std::stringstream ss;
  driver.print(ss);
  EXPECT_EQ(ss.str(),
            "((assign arr (1 2 3 4 5)) (for i arr (assign main i)))");
}
TEST(bison_parser_test, comment) {
    driver.clear();
  driver.parsefile("test_comment.mmm");
  std::stringstream ss;
  driver.print(ss);
  EXPECT_EQ(ss.str(),
            "(assign main 1)");
}

TEST(bison_parser_test, typeidentifier) {
    driver.clear();
  driver.parsefile("test_typeident.mmm");
  std::stringstream ss;
  driver.print(ss);
  EXPECT_EQ(ss.str(),
            "((assign add (lambda (x y) (return (+ x y)))) (assign add (lambda (x y) (return (+ x y)))) (assign hof (lambda (x y) (return (y (x))))) (assign hof2 (lambda (x y) ((assign localvar 1) (return (lambda (x) (return (y (x localvar)))))))))");
}

TEST(JSON_test, basictest) {
  driver.clear();

  driver.parsefile("test_if_nested.mmm");
  std::stringstream ss;
  driver.printJson(ss);
  EXPECT_EQ(ss.str(),
            "[[[ 'assign' ,'test', [ 'lambda' ,['x' , 'y' , 'z'], [['if' ,'x', [[ 'assign' ,'res', 0]], [['if' ,'y', [[ 'assign' ,'res', 100]], [[ 'assign' ,'res', 'z']]]]] , [ 'return' ,'res']]]] , [ 'assign' ,'zero', ['test', [1 , 23 , 244]]] , [ 'assign' ,'hand', ['test', [0 , 200 , 400]]] , [ 'assign' ,'fivehand', ['test', [0 , 0 , 500]]]]]");
}