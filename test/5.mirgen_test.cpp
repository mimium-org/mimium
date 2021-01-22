#include "basic/ast_to_string.hpp"
#include "compiler/ast_loader.hpp"
#include "compiler/mirgenerator.hpp"
#include "compiler/symbolrenamer.hpp"
#include "compiler/type_infer_visitor.hpp"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"

#define PREP(FILENAME)                                        \
  Driver driver{};                                            \
  ast::Statements& ast = *driver.parseFile(TEST_ROOT_DIR "/" #FILENAME ".mmm"); \
  SymbolRenamer renamer;                                      \
  auto newast = renamer.rename(ast);                          \
  TypeInferer inferer;                                        \
  auto& env = inferer.infer(*newast);                         \
  MirGenerator mirgenerator(env);

TEST(mirgen, basic) {
  PREP(test_localvar)
  auto mir = mirgenerator.generate(*newast);
  std::string target = R"(root:
  hoge0 = fun x1 , y2
  hoge0:
    allocate localvar3 : float*
    k0 = 2.000000
    store k0 to localvar3
    k2 = Mul x1 y2
    k3 = load localvar3
    k1 = Add k2 k3
    return k1

  allocate main4 : float*
  k6 = 5.000000
  k7 = 7.000000
  k5 = appcls hoge0 k6 , k7
  store k5 to main4
)";
  EXPECT_EQ(mir::toString(mir), target);
}