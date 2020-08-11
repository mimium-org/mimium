#include "basic/ast_to_string.hpp"

#include "basic/ast_new.hpp"
#include "compiler/ast_loader_new.hpp"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
namespace mimium {

TEST(asttostring, basic) {
  newast::DebugInfo dbg;
  auto statement = newast::makeStatement(
      newast::Assign{dbg,
                     {dbg, {"hoge"}, {std::optional(types::Float())}},
                     newast::makeExpr(newast::Number{dbg, 1})});
  std::ostringstream ss;
  std::visit(StatementStringVisitor(ss), *statement);
  std::string target("(assign (lvar hoge float) 1)");
  EXPECT_STREQ(ss.str().c_str(), target.c_str());
}

TEST(asttostring_parser, basic) {
  Driver driver{};
  auto ast = driver.parseFile("parser/ifstmt.mmm");
  std::ostringstream ss;
  ss << *ast;
  std::string target(
      "(assign"
      " (lvar test unspecified)"
      " (lambda ((lvar x unspecified) "
      "(lvar y unspecified) (lvar z unspecified))"
      "\n(if x "
      "(assign (lvar res unspecified) 0) "
      "(if y (assign (lvar res unspecified) 100) "
      "(assign (lvar res unspecified) z))"
      ")(return res)))");
  EXPECT_STREQ(ss.str().c_str(), target.c_str());
}

}  // namespace mimium