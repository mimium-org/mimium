#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
#include "basic/ast_new.hpp"
namespace mimium{

TEST(newast_test, basic) {
    newast::DebugInfo dbg;
    newast::Number num1 = {{dbg},1};
    auto numptr = makeExpr(newast::Number{dbg,1});
    auto strptr = makeExpr(newast::String{dbg,"test!"});
    newast::String str = {dbg,"test"};
    newast::Op opast = {{dbg},newast::OpId::Add,numptr,strptr};
    auto variant = *opast.lhs.value();
    EXPECT_TRUE(std::holds_alternative<newast::Number>(variant));
    double target = std::get<newast::Number>(variant).value;
    EXPECT_EQ(target,1.0);
    auto str_answer = std::get<newast::String>(*opast.rhs).value;
    std::string str_target = "test!";
    EXPECT_EQ(str_answer,str_target);
}

}