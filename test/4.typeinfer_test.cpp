#include "compiler/ast_loader.hpp"
#include "compiler/ast_lowering.hpp"
#include "compiler/scanner.hpp"
#include "compiler/type_infer_visitor.hpp"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
#include "mimium_parser.hpp"

#define PREP(FILENAME)                                                           \
  Driver driver{};                                                               \
  auto ast = driver.parseFile(TEST_ROOT_DIR "/typeinference/" #FILENAME ".mmm"); \
  lowerast::AstLowerer lowerer;                                                  \
  auto newast = lowerer.lowerHast(ast);                                          \
  TypeInferer inferer;                                                           \
  auto FType = HType::Value{HType::Float{}};
namespace mimium {
TEST(typeinfer, float_success) {  // NOLINT
  PREP(float_success)
  auto tenv = inferer.infer(newast);
  auto t = tenv.search("hoge0");
  EXPECT_TRUE(t.has_value());
  EXPECT_TRUE(std::holds_alternative<HType::Float>(t.value().v));
}
TEST(typeinfer, float_infer) {  // NOLINT
  PREP(float_infer)
  auto tenv = inferer.infer(newast);
  auto t = tenv.search("hoge0");
  EXPECT_TRUE(t.has_value());
  EXPECT_TRUE(std::holds_alternative<HType::Float>(t.value().v));
}
TEST(typeinfer, float_failure) {  // NOLINT
  PREP(float_failure)
  EXPECT_THROW(auto tenv = inferer.infer(newast);, std::runtime_error);  // NOLINT
}
TEST(typeinfer, function) {  // NOLINT
  PREP(function)
  auto tenv = inferer.infer(newast);
  auto add = tenv.search("add0").value();
  auto add_a = tenv.search("a1").value();
  auto add_b = tenv.search("b2").value();
  auto muladd = tenv.search("muladd3").value();
  auto muladd_a = tenv.search("a4").value();
  auto muladd_b = tenv.search("b5").value();
  auto muladd_c = tenv.search("c6").value();
  auto res = tenv.search("result7").value();

  EXPECT_TRUE(std::holds_alternative<HType::Float>(add_a.v));
  EXPECT_TRUE(std::holds_alternative<HType::Float>(add_b.v));
  EXPECT_TRUE(std::holds_alternative<HType::Function>(add.v));
  // EXPECT_TRUE((std::get<HType::Function>(add.v).v ==
  //              std::pair(List<Box<HType::Value>>{FType, FType}, Box(FType))));
  EXPECT_TRUE(std::holds_alternative<HType::Float>(muladd_a.v));
  EXPECT_TRUE(std::holds_alternative<HType::Float>(muladd_b.v));
  EXPECT_TRUE(std::holds_alternative<HType::Float>(muladd_c.v));
  EXPECT_TRUE(std::holds_alternative<HType::Function>(muladd.v));
  // EXPECT_TRUE((std::get<HType::Function>(muladd.v).v ==
  //              std::pair(List<Box<HType::Value>>{FType, FType, FType}, Box(FType))));
  EXPECT_TRUE(std::holds_alternative<HType::Float>(res.v));
}
// TEST(typeinfer, highorderfunction) {  // NOLINT
//   PREP(highorderfunction)
//   auto tenv = inferer.infer(newast);
//   auto add = tenv.search("add0");
//   EXPECT_TRUE(std::holds_alternative<HType::Float>(tenv.search("x1").value().v));
//   EXPECT_TRUE(std::holds_alternative<HType::Float>(tenv.search("y2").value().v));
//   auto add2type = std::pair(List<Box<HType::Value>>{FType, FType}, Box(FType));
//   EXPECT_TRUE(std::holds_alternative<HType::Function>(add.value().v));
//   EXPECT_TRUE(std::get<HType::Function>(add.value().v).v == add2type);
//   auto hof = tenv.search("hof3");
//   EXPECT_TRUE(std::holds_alternative<HType::Float>(tenv.search("x4").value().v));
//   EXPECT_TRUE(std::holds_alternative<HType::Function>(tenv.search("y5").value().v));
//   EXPECT_TRUE(std::holds_alternative<HType::Function>(hof.value().v));
//   auto hofrettype = std::pair(List<Box<HType::Value>>{FType}, Box(FType));
//   auto hof_target_t =
//       HType::Function{std::pair(List<Box<HType::Value>>{HType::Value{HType::Float{}},
//                                                         HType::Value{HType::Function{add2type}}},
//                                 Box(HType::Value{HType::Function{hofrettype}}))};
//   EXPECT_TRUE((std::get<HType::Function>(hof.value().v).v == hof_target_t.v));
//   EXPECT_TRUE(std::holds_alternative<HType::Float>(tenv.search("result").value().v));
// }

// TEST(typeinfer, recursivecall) {  // NOLINT
//   PREP(recursivecall)
//   auto tenv = inferer.infer(newast);
//   auto fntype = std::pair(List<Box<HType::Value>>{FType}, Box(FType));
//   EXPECT_TRUE(std::get<HType::Function>(tenv.search("fact").value().v).v == fntype);
// }
// TEST(typeinfer, self) {  // NOLINT
//   PREP(self)
//   auto tenv = inferer.infer(newast);
//   auto fntype = std::pair(List<Box<HType::Value>>{FType, FType}, Box(FType));
//   EXPECT_TRUE(std::get<HType::Function>(tenv.search("lowpass").value().v).v == fntype);
// }

// TEST(typeinfer, occurfail) {  // NOLINT
//   PREP(occur_failure)
//   EXPECT_THROW(auto tenv = inferer.infer(newast);, std::runtime_error);  // NOLINT
// }

}  // namespace mimium