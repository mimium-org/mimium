#include "basic/ast.hpp"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
namespace mimium {

TEST(newast, basic) {
  ast::DebugInfo dbg;
  ast::Number num1 = {{dbg}, 1};
  auto numptr = makeExpr(ast::Number{dbg, 1});
  auto strptr = makeExpr(ast::String{dbg, "test!"});
  ast::String str = {dbg, "test"};
  ast::Op opast = {{dbg}, ast::OpId::Add, numptr, strptr};
  auto variant = *opast.lhs.value();
  EXPECT_TRUE(std::holds_alternative<ast::Number>(variant));
  double target = std::get<ast::Number>(variant).value;
  EXPECT_EQ(target, 1.0);
  auto str_answer = std::get<ast::String>(*opast.rhs).value;
  std::string str_target = "test!";
  EXPECT_EQ(str_answer, str_target);
}
TEST(newast, statement) {
  ast::DebugInfo dbg;
  auto statement =
      ast::makeStatement(ast::Assign{dbg,
                                     {dbg, {"leftvar"}, {std::optional(types::Float{})}},
                                     ast::makeExpr(ast::Number{dbg, 1})});
  EXPECT_TRUE(std::holds_alternative<ast::Assign>(*statement));
  // EXPECT_EQ(std::get<ast::Assign>(*statement).lvar.type.value() ,types::Value(types::Float{}));
}

}  // namespace mimium