
#include "mimium_parser.hpp"
#include "compiler/scanner.hpp"
#include "compiler/ast_lowering.hpp"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"

namespace mimium {
TEST(symbolrename, astcomplete) {//NOLINT
  Driver driver{};
  auto ast = driver.parseFile(TEST_ROOT_DIR "/ast_complete.mmm");
  lowerast::AstLowerer lowerer;
  auto newast = lowerer.lowerHast(ast);
  auto res = toString<LAst>(newast);

  std::string target =
R"((assign (lvar myvar0 float) (Mod (Exponent (Add 100 (Div (Mul (Sub 200 300) 20) 15)) 2) 30.2))
(assign (lvar inlinefn1 unspecified) (lambda ((lvar myarg2 float))(Mul myarg2 2)))
(Fdef (lvar blockfn3 unspecified) (lambda ((lvar x4 float) (lvar y5 float) (lvar z6 float))(return (if (GreaterThan x4 25) y5 z6))
))
(assign (lvar result7 float) (Add (funcall inlinefn1 (10)) (funcall blockfn3 (20 333 555))))
(Fdef (lvar lowpass_op8 unspecified) (lambda ((lvar input9 float) (lvar fb10 float))(return (Add (Mul (Sub 1 fb10) input9) (Mul fb10 self)))
))
(Fdef (lvar loop11 unspecified) (lambda ((lvar input12 float))(funcall println (input12))
(time (funcall loop11 ((Add input12 100))) )
))
(Fdef (lvar iteration13 unspecified) (lambda ((lvar input14 float))(assign (lvar myarray15 unspecified) (array (2 4 6 8 10) ))

))
(Fdef (lvar conditional17 unspecified) (lambda ()(assign (lvar hoge18 unspecified) 30)
(if (LessThan hoge18 20) (funcall println (hoge18)))
(if (GreaterThan hoge18 25) (funcall println ((Mul hoge18 100))) (funcall println ((Mul hoge18 1000))))))
)";
  EXPECT_STRCASEEQ(res.c_str(), target.c_str());
}

}  // namespace mimium