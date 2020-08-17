#include "basic/ast_to_string.hpp"
#include "compiler/alphaconvert_visitor.hpp"
#include "compiler/ast_loader_new.hpp"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"

namespace mimium {
TEST(symbolrename, astcomplete) {
  Driver driver{};
  auto ast = driver.parseFile("ast_complete.mmm");
  SymbolRenamer renamer;
  auto newast = renamer.rename(*ast);
  std::ostringstream ss;
  ss << *newast;
  std::string target =
      "(assign (lvar myvar0 float) (Mod (Exponent (Add 100 (Div (Mul (Sub 200 300) 20) 15)) 2) 30.2))\n(assign (lvar inlinefn2 unspecified) (lambda ((lvar myarg1 unspecified))(Mul myarg1 2)\n))\n(assign (lvar blockfn6 unspecified) (lambda ((lvar x3 unspecified) (lvar y4 unspecified) (lvar z5 unspecified))(return (funcall ifexpr ((GreaterThan x3 25) y4 z5)))\n))\n(assign (lvar result7 float) (Add (funcall inlinefn2 (10)) (funcall blockfn6 (20 333 555))))\n(assign (lvar lowpass_op10 unspecified) (lambda ((lvar input8 unspecified) (lvar fb9 unspecified))(return (Add (Mul (Sub 1 fb9) input8) (Mul fb9 self)))\n))\n(assign (lvar loop12 unspecified) (lambda ((lvar input11 unspecified))(funcall println (input11))\n(time (funcall loop ((Add input11 100))) )\n))\n(assign (lvar iteration16 unspecified) (lambda ((lvar input13 unspecified))(assign (lvar myarray14 unspecified) (array (2 4 6 8 10) ))\n\n))\n(assign (lvar conditional18 unspecified) (lambda ()(assign (lvar hoge17 unspecified) 30)\n(if (LessThan hoge17 20) (funcall println (hoge17))\n)\n(if (GreaterThan hoge17 25) (funcall println ((Mul hoge17 100)))\n (funcall println ((Mul hoge17 1000)))\n)\n))\n";
  EXPECT_STRCASEEQ(ss.str().c_str(), target.c_str());
}

}  // namespace mimium