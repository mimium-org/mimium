
#include "basic/ast_new.hpp"
#include "compiler/ast_loader.hpp"

#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
namespace mimium {

TEST(asttostring, basic) {//NOLINT
DebugInfo dbg;

  auto opast = Hast::expr{Hast::Infix{"+", Hast::expr{Hast::FloatLit{1.0, dbg}},
                           Hast::expr{Hast::StringLit{"test", dbg}}, dbg}};
  std::ostringstream ss;
  std::string target("(infix + 1.0 test)");
  EXPECT_STREQ(toString<Hast>(opast).c_str(), target.c_str());
}

// TEST(asttostring_parser, basic) {
//   Driver driver{};
//   auto ast = driver.parseFile("parser/ifstmt.mmm");
//   std::ostringstream ss;
//   ss << *ast;
//   std::string target(
//       "(assign (lvar test unspecified) (lambda ((lvar x unspecified) (lvar y "
//       "unspecified) (lvar z unspecified))(if x (assign (lvar res unspecified) "
//       "0)\n (if y (assign (lvar res unspecified) 100)\n (assign (lvar res "
//       "unspecified) z)\n)\n)\n(return res)\n))\n");
//   EXPECT_STREQ(ss.str().c_str(), target.c_str());
// }

}  // namespace mimium