#include "basic/ast_new.hpp"
#include "compiler/ast_lowering.hpp"
#include "gtest/gtest.h"
#include "gtest/internal/gtest-port.h"
namespace mimium {


TEST(astlower, basic) {  // NOLINT

  DebugInfo dbg;
  auto opast = Hast::Infix{"+", Hast::expr{Hast::FloatLit{1.0, dbg}},
                           Hast::expr{Hast::StringLit{"test!", dbg}}, dbg};
    auto statement = Hast::Statement{ Hast::Assignment{Hast::Id{"hoge"},Hast::expr{opast} }};
    auto statement2 = Hast::Statement{ Hast::Assignment{Hast::Id{"hoge2"},Hast::expr{opast} }};
    auto stmts = List<decltype(statement)>{statement,statement2};
    auto top = Hast::expr{Hast::Block{stmts}};
    lowerast::AstLowerer lowerer;
    auto  env = std::make_shared<lowerast::AstLowerer::Env>();
    auto res = lowerer.lowerHast(top,env);
    std::cout << mimium::toString<LAst>(res) <<std::endl;
}
}