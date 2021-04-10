#pragma once
#include "ast_new.hpp"

namespace mimium {
template <class EXPR>
struct MacroPattern {
  using matcher = bool(EXPR);
};
namespace lowerast {

using TypeEnv = TypeEnv<Hast::expr, IType::Value>::Value;
LAst::expr lowerHast(Hast::expr const& expr, TypeEnv const& env);
}  // namespace lowerast
}  // namespace mimium