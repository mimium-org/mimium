#pragma once
#include "basic/ast_new.hpp"
#include "basic/environment.hpp"
namespace mimium {

namespace lowerast {
// op->関数名テーブル？
class AstLowerer {
 public:
  using Id = std::string;
  using Env = Environment<std::string, Identifier>;
  using EnvPtr = std::shared_ptr<Env>;
  LAst::expr lowerHast(Hast::expr const& expr, EnvPtr& env);
  template <class T>
  LAst::expr makeExpr(T&& a) {
    return LAst::expr{std::forward<T>(a), expr_uid++};
  };

 private:
  using exprfunction = std::function<LAst::expr(LAst::expr)>;
  LAst::expr lowerHStatements(const List<Hast::Expr::Statement>& stmts, EnvPtr env);
  using stmt_iter_t = List<Hast::Expr::Statement>::const_iterator;
  LAst::expr lowerHStatement(Hast::Expr::Statement const& s, stmt_iter_t const& next_s,
                             stmt_iter_t const& end_iter, EnvPtr env);
  LAst::Lvar lowerLvar(Hast::Lvar const& lv);
  int64_t makeUidforId() { return id_uid++; };
  Identifier makeId(std::string const& id);
  int expr_uid = 0;
  int64_t id_uid = 0;
};

// LAst::expr removeSelf(LAst::expr const& expr);

}  // namespace lowerast
}  // namespace mimium