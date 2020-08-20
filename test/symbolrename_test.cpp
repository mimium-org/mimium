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
      "(assign (lvar myvar0 float) (Mod (Exponent (Add 100 (Div (Mul (Sub 200 300) 20) 15)) 2) 30.2))\n(assign (lvar inlinefn1 unspecified) (lambda ((lvar myarg2 float))(Mul myarg2 2)\n))\n(assign (lvar blockfn3 unspecified) (lambda ((lvar x4 float) (lvar y5 float) (lvar z6 float))(return (funcall ifexpr ((GreaterThan x4 25) y5 z6)))\n))\n(assign (lvar result7 float) (Add (funcall inlinefn1 (10)) (funcall blockfn3 (20 333 555))))\n(assign (lvar lowpass_op8 unspecified) (lambda ((lvar input9 float) (lvar fb10 float))(return (Add (Mul (Sub 1 fb10) input9) (Mul fb10 self)))\n))\n(assign (lvar loop11 unspecified) (lambda ((lvar input12 float))(funcall println (input12))\n(time (funcall loop11 ((Add input12 100))) )\n))\n(assign (lvar iteration13 unspecified) (lambda ((lvar input14 float))(assign (lvar myarray15 unspecified) (array (2 4 6 8 10) ))\n\n))\n(assign (lvar conditional17 unspecified) (lambda ()(assign (lvar hoge18 unspecified) 30)\n(if (LessThan hoge18 20) (funcall println (hoge18))\n)\n(if (GreaterThan hoge18 25) (funcall println ((Mul hoge18 100)))\n (funcall println ((Mul hoge18 1000)))\n)\n))\n";
  EXPECT_STRCASEEQ(ss.str().c_str(), target.c_str());
}

}  // namespace mimium