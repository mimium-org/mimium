#include "basic/ast_new.hpp"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
namespace mimium {



TEST(newast, basic) {  // NOLINT
  DebugInfo dbg;

  auto opast = Hast::Infix{"+", Hast::expr{Hast::FloatLit{1.0, dbg}},
                           Hast::expr{Hast::StringLit{"test!", dbg}}, dbg};
  auto variant = opast.v.lhs.value();
  EXPECT_TRUE(std::holds_alternative<Hast::FloatLit>(variant.getraw().v));
  double target = std::get<Hast::FloatLit>(variant.getraw().v).v;
  EXPECT_EQ(target, 1.0);
  auto str_answer = std::get<Hast::StringLit>(opast.v.rhs.getraw().v).v;
  std::string str_target = "test!";
  EXPECT_EQ(str_answer, str_target);
}
// TODO: more tests

}  // namespace mimium