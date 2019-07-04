// #define MIMIUM_DEBUG 4

#include "gtest/gtest.h"
#include "driver.hpp"


TEST(bison_parser_test, assign) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "a = 1";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign a 1) )");
}
TEST(bison_parser_test, assignfpoint) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "a = 1.245";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign a 1.245) )");
}

TEST(bison_parser_test, expr) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "a = 1+2+3*2/2";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign a (+ (+ 1 2) (/ (* 3 2) 2))) )");
}

TEST(bison_parser_test, parensis) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "a=(2+3)/5";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign a (/ (+ 2 3) 5)) )");
}

TEST(bison_parser_test, lines) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "a=2 \n b=3";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign a 2) (assign b 3) )");
}

TEST(bison_parser_test, fdef) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "fn myfunc(a,b){return 1+2}";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign myfunc (lambda ((a b ))(( return (+ 1 2)) ))) )");
}
TEST(bison_parser_test, fdef_multi) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "fn myfunc(a,b){\n return 1+2 \n} \n fn myfuncb(c,d){\nreturn 1+2\n}";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign myfunc (lambda ((a b ))(( return (+ 1 2)) ))) (assign myfuncb (lambda ((c d ))(( return (+ 1 2)) ))) )");
}
TEST(bison_parser_test, fdef_multi2) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = \
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
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign makecounter (lambda ((x ))((assign localvar 0) (assign countup (lambda ((y ))((assign localvar (+ localvar x)) ( return localvar) ))) ( return countup) ))) (assign ctr (makecounter (1 ))) (assign rec (lambda ((y ))((assign myctr (ctr (1 ))) (assign test (println (myctr ))@x) ( return (rec (x ))) ))) (assign main (rec (48000 ))@0) )");
}



TEST(bison_parser_test, lambda) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "myfunc = (a,b)->{1+2}";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign myfunc (lambda ((a b ))(+ 1 2))) )");
}


TEST(bison_parser_test, time) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "a = (3+2)@50";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign a (+ 3 2)@50) )");
}


TEST(bison_parser_test, fileread) {
     mmmpsr::MimiumDriver driver;
     driver.parsefile("testfile1.mmm");
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign a (+ 2 3)@128) )");
}

TEST(bison_parser_test, func_block) {
          mmmpsr::MimiumDriver driver;
     driver.parsefile("testfile_blockfun.mmm");
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign hoge (lambda ((x y ))(( return 1) ))) )");
}

TEST(bison_parser_test, statements) {
          mmmpsr::MimiumDriver driver;
     driver.parsefile("testfile_statements.mmm");
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign hoge (lambda ((x y ))(( return (+ x y)) ))) (assign main (hoge (3 5 ))) )");
}
TEST(bison_parser_test, nestfunction) {
          mmmpsr::MimiumDriver driver;
     driver.parsefile("test_localvar2.mmm");
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign hoge (lambda ((x ))((assign localvar x) (assign fuga (lambda ((y ))((assign localvar (+ localvar y)) ( return localvar) ))) ( return (fuga (2.5 ))) ))) (assign main (hoge (1 ))) )");
}

TEST(bison_parser_test, ifstatement) {
     mmmpsr::MimiumDriver driver;
     driver.parsefile("test_if.mmm");
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign test (lambda ((x y ))(if x ((assign res y) ) ((assign res 100) ) ( return res) ))) (assign true (test (1 5 ))) (assign false (test (0 100 ))) )");
}
TEST(bison_parser_test, ifstatement2) {
     mmmpsr::MimiumDriver driver;
     driver.parsefile("test_if_nested.mmm");
     std::stringstream ss;
     driver.print(ss);
     std::cout << ss.str()<<std::endl;
     EXPECT_EQ(ss.str(),"((assign test (lambda ((x y z ))(if x ((assign res 0) ) (if y ((assign res 100) ) ((assign res z) ) ) ( return res) ))) (assign zero (test (1 23 244 ))) (assign hand (test (0 200 400 ))) (assign fivehand (test (0 0 500 ))) )");
}
TEST(bison_parser_test, array) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "main = [1,2,3,4,5]";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     EXPECT_EQ(ss.str(),"((assign main (1 2 3 4 5 )) )");

}
TEST(bison_parser_test, arrayaccess) {
     mmmpsr::MimiumDriver driver;
     std::string teststr = "arr = [1,2,3,4,5]\n\
                         main = arr[2]";
     driver.parsestring(teststr);
     std::stringstream ss;
     driver.print(ss);
     EXPECT_EQ(ss.str(),"((assign arr (1 2 3 4 5 )) (assign main arrayaccess arr 2) )");

}