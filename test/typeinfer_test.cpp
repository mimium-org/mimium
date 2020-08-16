#include "basic/ast_to_string.hpp"
#include "compiler/alphaconvert_visitor.hpp"
#include "compiler/ast_loader_new.hpp"
#include "compiler/type_infer_visitor.hpp"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"

#define PREP(FILENAME)                                      \
  Driver driver{};                                          \
  newast::Statements& ast =                                 \
      *driver.parseFile("typeinference/" #FILENAME ".mmm"); \
  SymbolRenamer renamer;                                    \
  auto newast = renamer.rename(ast);                        \
  TypeInferer inferer;
namespace mimium {
TEST(typeinfer, float_success) {
  PREP(float_success)
  EXPECT_TRUE(std::holds_alternative<types::Float>(
      inferer.infer(*newast).env.at("hoge0")));
}
TEST(typeinfer, float_infer) {
  PREP(float_infer)
  EXPECT_TRUE(std::holds_alternative<types::Float>(
      inferer.infer(*newast).env.at("hoge0")));
}
TEST(typeinfer, float_failure) {
  PREP(float_failure)
  EXPECT_THROW(auto a = inferer.infer(*newast);, std::runtime_error);
}
TEST(typeinfer, function) {
  PREP(function)
  auto& env = inferer.infer(*newast).env;
  auto& add_a = env.at("a0");
  auto& add_b = env.at("b1");
  auto& add = env.at("add2");
  auto& muladd_a = env.at("a3");
  auto& muladd_b = env.at("b4");
  auto& muladd_c = env.at("c5");
  auto& muladd = env.at("muladd6");
  auto& res = env.at("result7");

  EXPECT_TRUE(std::holds_alternative<types::rFunction>(add));
  EXPECT_TRUE(std::holds_alternative<types::Float>(add_a));
  EXPECT_TRUE(std::holds_alternative<types::Float>(add_b));
  EXPECT_TRUE(std::holds_alternative<types::rFunction>(muladd));
  EXPECT_TRUE(std::holds_alternative<types::Float>(muladd_a));
  EXPECT_TRUE(std::holds_alternative<types::Float>(muladd_b));
  EXPECT_TRUE(std::holds_alternative<types::Float>(muladd_c));
  EXPECT_TRUE(std::holds_alternative<types::Float>(res));

  EXPECT_TRUE(
      rv::get<types::Function>(add) ==
      types::Function(types::Float(), {types::Float(), types::Float()}));
  EXPECT_TRUE(rv::get<types::Function>(muladd) ==
              types::Function(types::Float(), {types::Float(), types::Float(),
                                               types::Float()}));
}
}  // namespace mimium