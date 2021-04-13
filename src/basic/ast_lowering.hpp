#pragma once
#include "ast_new.hpp"
#include "environment.hpp"
namespace mimium {
template <class EXPR>
struct MacroPattern {
  using matcher = bool(EXPR);
};
namespace lowerast {

LAst::expr lowerHast(Hast::expr const& expr);
}  // namespace lowerast
}  // namespace mimium