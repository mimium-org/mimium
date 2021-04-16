#pragma once
#include "basic/ast_new.hpp"
#include "basic/environment.hpp"
namespace mimium {
template <class EXPR>
struct MacroPattern {
  using matcher = bool(EXPR);
};
namespace lowerast {
// op->関数名テーブル？
class AstLowerer {
 public:
  LAst::expr lowerHast(Hast::expr const& expr);
  template <class T>
  LAst::expr makeExpr(T&& a) {
    return LAst::expr{a, uniqueid++};
  };
  private:
  using exprfunction = std::function<LAst::expr(LAst::expr)>;
  LAst::expr lowerHStatements(const List<Hast::Expr::Statement>& stmts);
  using Id = std::string;
  using EnvPtr = std::shared_ptr<Environment<Id, LAst::expr>>;
  std::pair<exprfunction,EnvPtr> lowerHStatement(Hast::Expr::Statement const& s, EnvPtr env);

  int uniqueid = 0;
};

LAst::expr removeSelf(LAst::expr const& expr);

}  // namespace lowerast
}  // namespace mimium