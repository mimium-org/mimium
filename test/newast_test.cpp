#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
#include "basic/ast_new.hpp"
namespace mimium{

TEST(newast, basic) {
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
TEST(newast, statement) {
    newast::DebugInfo dbg;
    auto statement = newast::makeStatement(newast::Assign{dbg,
    {dbg,{"leftvar"},{std::optional(types::Float())}},
    newast::makeExpr(newast::Number{dbg,1})
    } );
    EXPECT_TRUE(std::holds_alternative<newast::Assign>(*statement));
    EXPECT_EQ(std::get<newast::Assign>(*statement).lvar.type.value() ,types::Value(types::Float()));
}   

}