#include "basic/ast_to_string.hpp"

#include "basic/ast.hpp"
#include "compiler/ast_loader.hpp"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
namespace mimium {

TEST(asttostring, basic) {
  ast::DebugInfo dbg;
  auto statement = ast::makeStatement(
      ast::Assign{dbg,
                     {dbg, {"hoge"}, {std::optional(types::Float())}},
                     ast::makeExpr(ast::Number{dbg, 1})});
  std::ostringstream ss;
  std::visit(StatementStringVisitor(ss), *statement);
  std::string target("(assign (lvar hoge float) 1)");
  EXPECT_STREQ(ss.str().c_str(), target.c_str());
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