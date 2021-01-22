#include "basic/ast_to_string.hpp"
#include "compiler/ast_loader.hpp"
#include "compiler/symbolrenamer.hpp"
#include "compiler/type_infer_visitor.hpp"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"

#define PREP(FILENAME)                                                                        \
  Driver driver{};                                                                            \
  ast::Statements& ast = *driver.parseFile(TEST_ROOT_DIR "/typeinference/" #FILENAME ".mmm"); \
  SymbolRenamer renamer;                                                                      \
  auto newast = renamer.rename(ast);                                                          \
  TypeInferer inferer;
namespace mimium {
TEST(typeinfer, float_success) {//NOLINT
  PREP(float_success)
  EXPECT_TRUE(std::holds_alternative<types::Float>(inferer.infer(*newast).env.at("hoge0")));
}
TEST(typeinfer, float_infer) {//NOLINT
  PREP(float_infer)
  EXPECT_TRUE(std::holds_alternative<types::Float>(inferer.infer(*newast).env.at("hoge0")));
}
TEST(typeinfer, float_failure) {//NOLINT
  PREP(float_failure)
  EXPECT_THROW(auto a = inferer.infer(*newast);, std::runtime_error);//NOLINT
}
TEST(typeinfer, function) {//NOLINT
  PREP(function)
  auto& env = inferer.infer(*newast).env;
  auto& add = env.at("add0");
  auto& add_a = env.at("a1");
  auto& add_b = env.at("b2");
  auto& muladd = env.at("muladd3");
  auto& muladd_a = env.at("a4");
  auto& muladd_b = env.at("b5");
  auto& muladd_c = env.at("c6");
  auto& res = env.at("result7");

  EXPECT_TRUE(std::holds_alternative<types::Float>(add_a));
  EXPECT_TRUE(std::holds_alternative<types::Float>(add_b));
  EXPECT_TRUE(std::holds_alternative<types::rFunction>(add));
  EXPECT_TRUE((rv::get<types::Function>(add) ==
               types::Function{types::Float{}, {types::Float{}, types::Float{}}}));
  EXPECT_TRUE(std::holds_alternative<types::Float>(muladd_a));
  EXPECT_TRUE(std::holds_alternative<types::Float>(muladd_b));
  EXPECT_TRUE(std::holds_alternative<types::Float>(muladd_c));
  EXPECT_TRUE(std::holds_alternative<types::rFunction>(muladd));
  EXPECT_TRUE((rv::get<types::Function>(muladd) ==
               types::Function{types::Float{}, {types::Float{}, types::Float{}, types::Float{}}}));
  EXPECT_TRUE(std::holds_alternative<types::Float>(res));
}
TEST(typeinfer, highorderfunction) {//NOLINT
  PREP(highorderfunction)
  auto& env = inferer.infer(*newast).env;
  auto& add = env.at("add0");
  EXPECT_TRUE(std::holds_alternative<types::Float>(env.at("x1")));
  EXPECT_TRUE(std::holds_alternative<types::Float>(env.at("y2")));
  auto add2type = types::Function{types::Float{}, {types::Float{}, types::Float{}}};
  EXPECT_TRUE(std::holds_alternative<types::rFunction>(add));
  EXPECT_TRUE(rv::get<types::Function>(add) == add2type);
  auto& hof = env.at("hof3");
  EXPECT_TRUE(std::holds_alternative<types::Float>(env.at("x4")));
  EXPECT_TRUE(std::holds_alternative<types::rFunction>(env.at("y5")));
  EXPECT_TRUE(std::holds_alternative<types::rFunction>(hof));
  auto hofrettype = types::Function{types::Float{}, {types::Float{}}};

  EXPECT_TRUE(
      (rv::get<types::Function>(hof) == types::Function{hofrettype, {types::Float{}, add2type}}));
  EXPECT_TRUE(std::holds_alternative<types::Float>(env.at("result7")));
}

TEST(typeinfer, recursivecall) {//NOLINT
  PREP(recursivecall)
  auto& env = inferer.infer(*newast).env;
  EXPECT_TRUE((rv::get<types::Function>(env.at("fact0")) ==
               types::Function{types::Float{}, {types::Float{}}}));
}
TEST(typeinfer, self) {//NOLINT
  PREP(self)
  auto& env = inferer.infer(*newast).env;
  EXPECT_TRUE((rv::get<types::Function>(env.at("lowpass0")) ==
               types::Function{types::Float{}, {types::Float{}, types::Float{}}}));
}

TEST(typeinfer, occurfail) {//NOLINT
  PREP(occur_failure)
  EXPECT_THROW(auto a = inferer.infer(*newast);, std::runtime_error);//NOLINT
}

}  // namespace mimium