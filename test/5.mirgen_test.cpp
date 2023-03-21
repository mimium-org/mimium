#include "basic/mir.hpp"
#include "compiler/ast_loader.hpp"
#include "compiler/ast_lowering.hpp"
#include "compiler/mirgenerator.hpp"
#include "compiler/type_lowering.hpp"

#include "compiler/scanner.hpp"
#include "compiler/type_infer_visitor.hpp"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
#include "mimium_parser.hpp"

#define PREP(FILENAME)                                             \
  Driver driver{};                                                 \
  auto ast = driver.parseFile(TEST_ROOT_DIR "/" #FILENAME ".mmm"); \
  lowerast::AstLowerer lowerer;                                    \
  auto h_env = std::make_shared<lowerast::AstLowerer::Env>();        \
  auto newast = lowerer.lowerHast(ast, h_env);                       \
  TypeInferer inferer;                                             \
  auto env = inferer.infer(newast);

namespace mimium {

TEST(type, aggregatetype_check) {
  LType::Float ftype;
  LType::Value a{ftype};
  EXPECT_FALSE(isAggregate<LType>(a));
}

TEST(mirgen, basic) {  // NOLINT
  PREP(test_localvar)
  auto mir = mimium::generateMir(newast, env);
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
}  // namespace mimium