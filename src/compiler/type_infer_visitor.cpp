/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/type_infer_visitor.hpp"

namespace mimium {
// new TypeInferer
using ExprTypeVisitor = TypeInferer::ExprTypeVisitor;
using StatementTypeVisitor = TypeInferer::StatementTypeVisitor;
using LvarTypeVisitor = TypeInferer::LvarTypeVisitor;
using TypeUnifyVisitor = TypeInferer::TypeUnifyVisitor;

types::Value ExprTypeVisitor::operator()(ast::Op& ast) {
  return ast.lhs.has_value() ? inferer.unify(infer(ast.lhs.value()), infer(ast.rhs))
                             : infer(ast.rhs);
}
types::Value ExprTypeVisitor::operator()(ast::Number& ast) { return types::Float{}; }
types::Value ExprTypeVisitor::operator()(ast::String& ast) { return types::String{}; }
types::Value ExprTypeVisitor::operator()(ast::Symbol& ast) {
  return inferer.typeenv.find(ast.value);
}
types::Value ExprTypeVisitor::operator()(ast::Self& ast) {
  if (inferer.selftype_stack.empty()) {
    throw std::runtime_error("keyword \"self\" cannot be used out of function");
  }
  ast.type = inferer.selftype_stack.top();
  return inferer.selftype_stack.top();
}
types::Value ExprTypeVisitor::operator()(ast::Lambda& ast) {
  std::vector<types::Value> argtypes;
  for (auto&& a : ast.args.args) { argtypes.emplace_back(inferer.addDeclVar(a)); }

  auto rettype_tv = ast.ret_type.value_or(*inferer.typeenv.createNewTypeVar());
  inferer.selftype_stack.push(rettype_tv);
  auto rettype = (*this)(ast.body);
  auto uni_rettype = inferer.unify(rettype_tv, rettype);
  inferer.selftype_stack.pop();
  return types::Function{uni_rettype, argtypes};
}

types::Value TypeInferer::inferFcall(ast::Fcall& fcall) {
  std::vector<types::Value> argtypes;
  auto args = fcall.args.args;
  std::transform(args.begin(), args.end(), std::back_inserter(argtypes),
                 [&](ast::ExprPtr expr) { return exprvisitor.infer(expr); });
  auto frettype = *typeenv.createNewTypeVar();
  types::Value ftype = types::Function{frettype, argtypes};
  types::Value targettype = exprvisitor.infer(fcall.fn);
  auto res = unify(targettype, ftype);
  return rv::get<types::Function>(res).ret_type;
}

types::Value ExprTypeVisitor::operator()(ast::Fcall& ast) { return inferer.inferFcall(ast); }

types::Value ExprTypeVisitor::operator()(ast::Struct& ast) {
  std::vector<types::Struct::Keytype> argtypes;
  types::Value tmptype;
  for (auto&& a : ast.args) {
    // todo(tomoya): need to fix parser(add key)
    argtypes.push_back({"struct_key", std::visit(*this, *a)});
  }
  return types::Struct{std::move(argtypes)};
}
types::Value ExprTypeVisitor::operator()(ast::StructAccess& ast) {
  auto stru_type = std::visit(*this, *ast.stru);
  if (!rv::holds_alternative<types::Struct>(stru_type)) {
    throw std::runtime_error(
        "you cannot access to variables other than Struct type with dot "
        "operator.");
  }
  auto& stru = rv::get<types::Struct>(stru_type);
  // todo(tomoya): need to restrict field ast to rvar, not an term
  return stru.arg_types.at(0).val;
}
types::Value ExprTypeVisitor::operator()(ast::ArrayInit& ast) {
  std::vector<types::Value> argtypes;
  types::Value tmptype;
  types::Value atype = std::accumulate(
      ast.args.begin(), ast.args.end(), types::Value(*inferer.typeenv.createNewTypeVar()),
      [&](types::Value v, ast::ExprPtr e) { return inferer.unify(v, std::visit(*this, *e)); });
  return types::Array{std::move(atype), static_cast<int>(ast.args.size())};
}
types::Value ExprTypeVisitor::operator()(ast::ArrayAccess& ast) {
  auto indextype = inferer.unify(std::visit(*this, *ast.index), types::Value(types::Float{}));
  auto tv = inferer.typeenv.createNewTypeVar();
  auto arraytype =
      inferer.unify(std::visit(*this, *ast.array), types::Value(types::Array{*std::move(tv)}));
  if (!rv::holds_alternative<types::Array>(arraytype)) {
    throw std::runtime_error(
        "you cannot access to variables other than Array type with [] "
        "operator.");
  }
  return rv::get<types::Array>(arraytype).elem_type;
}
types::Value ExprTypeVisitor::operator()(ast::Tuple& ast) {
  // Todo(tomoya)
  types::Tuple res{};
  for (auto& a : ast.args) { res.arg_types.emplace_back(inferer.inferExpr(a)); }
  return res;
}

types::Value ExprTypeVisitor::operator()(ast::Block& ast) {
  auto stmttype = inferer.inferStatements(ast.stmts);
  return ast.expr.has_value() ? inferer.inferExpr(ast.expr.value()) : stmttype;
}
types::Value TypeInferer::inferIf(ast::If& ast) {
  if (!ast.else_stmts.has_value()) { return inferExpr(ast.then_stmts); }
  auto then_r_type = inferExpr(ast.then_stmts);
  auto else_r_type = inferExpr(ast.else_stmts.value());
  return unify(std::move(then_r_type), std::move(else_r_type));
}

types::Value ExprTypeVisitor::operator()(ast::If& ast) { return inferer.inferIf(ast); }

types::Value StatementTypeVisitor::operator()(ast::If& ast) { return inferer.inferIf(ast); }

types::Value StatementTypeVisitor::operator()(ast::Fdef& ast) {
  auto lvartype = inferer.addDeclVar(ast.lvar);
  auto rvartype = inferer.exprvisitor(ast.fun);
  inferer.unify(lvartype, rvartype);
  return types::Void{};
}

void LvarTypeVisitor::operator()(ast::DeclVar& ast) {
  auto lvartype = inferer.addDeclVar(ast);
  auto rvartype = inferer.inferExpr(rvar);
  inferer.typeenv.emplace(ast.value.value, inferer.unify(lvartype, rvartype));
}
void LvarTypeVisitor::operator()(ast::TupleLvar& ast) {
  std::vector<types::Value> typevec;
  for (auto&& a : ast.args) { typevec.push_back(inferer.addDeclVar(a)); }
  types::Tuple tuplelvar{typevec};
  auto rvartype = inferer.inferExpr(rvar);
  inferer.unify(tuplelvar, rvartype);
  auto rvartupletype = rv::get<types::Tuple>(rvartype);
  int count = 0;
  for (auto&& a : ast.args) {
    inferer.typeenv.emplace(a.value.value, rvartupletype.arg_types[count++]);
  }
}
void LvarTypeVisitor::operator()(ast::ArrayLvar& ast) {
  auto indextype = inferer.unify(inferer.inferExpr(ast.index), types::Value(types::Float{}));
  auto tv = inferer.typeenv.createNewTypeVar();
  auto arraytype =
      inferer.unify(inferer.inferExpr(ast.array), types::Value(types::Array{*std::move(tv)}));
  if (!rv::holds_alternative<types::Array>(arraytype)) {
    throw std::runtime_error(
        "you cannot access to variables other than Array type with [] "
        "operator.");
  }
  auto lvartype = rv::get<types::Array>(arraytype).elem_type;
  auto rvartype = inferer.inferExpr(rvar);
  inferer.unify(lvartype, rvartype);
}
types::Value StatementTypeVisitor::operator()(ast::Assign& ast) {
  LvarTypeVisitor assignvisitor(inferer, ast.expr);
  std::visit(assignvisitor, ast.lvar);
  return types::Void{};
}
types::Value StatementTypeVisitor::operator()(ast::Return& ast) {
  return inferer.inferExpr(ast.value);
}
// types::Value StatementTypeVisitor::operator()(ast::Declaration& ast){}
types::Value StatementTypeVisitor::operator()(ast::For& ast) {
  // TODO(tomoya): types for iterator
  inferer.exprvisitor(ast.statements);
  return types::Void{};  // for loop becomes void anyway
}

types::Value StatementTypeVisitor::operator()(ast::Fcall& ast) { return inferer.inferFcall(ast); }
types::Value StatementTypeVisitor::operator()(ast::Time& ast) {
  inferer.unify(inferer.inferExpr(ast.when), types::Float{});
  // anyway, this rvalue will not be used(time specification is used as void
  auto fcalltype = inferer.inferFcall(ast.fcall);
  return inferer.unify(fcalltype, types::Void{});
}

types::Value TypeInferer::addDeclVar(ast::DeclVar& lvar) {
  auto res = lvar.type.value_or(*typeenv.createNewTypeVar());
  auto [iter, was_newvar] = typeenv.emplace(lvar.value.value, res);
  return res;
}

types::Value TypeUnifyVisitor::unify(types::rTypeVar t1, types::rTypeVar t2) {
  types::Value res = t1;
  auto i1 = t1.getraw().index;
  auto i2 = t2.getraw().index;
  auto& t1_point = inferer.typeenv.tv_container[i1];
  auto& t2_point = inferer.typeenv.tv_container[i2];
  bool t1contain = !std::holds_alternative<types::rTypeVar>(t1_point);
  bool t2contain = !std::holds_alternative<types::rTypeVar>(t2_point);
  if (i1 != i2) {
    if (!t1contain && !t2contain) {
      if (i1 > i2) {
        t1_point = t2_point;
        res = t2_point;
      } else if (i1 < i2) {
        t2_point = t1_point;
        res = t1_point;
      }
    } else {
      res = inferer.unify(t1_point, t2_point);
    }
  }
  return res;
}

types::Value TypeUnifyVisitor::unify(types::rPointer p1, types::rPointer p2) {
  auto lhs = p1.getraw().val;
  auto rhs = p2.getraw().val;
  auto target = inferer.unify(std::move(lhs), std::move(rhs));
  return types::Pointer{target};
}
types::Value TypeUnifyVisitor::unify(types::rRef p1, types::rRef p2) {
  auto target = inferer.unify(p1.getraw().val, p2.getraw().val);
  return types::Ref{target};
}
types::Value TypeUnifyVisitor::unify(types::rAlias a1, types::rAlias a2) {
  return inferer.unify(a1.getraw().target, a2.getraw().target);
  ;
}
types::Value TypeUnifyVisitor::unify(types::rFunction f1, types::rFunction f2) {
  auto argtype = unifyArgs(f1.getraw().arg_types, f2.getraw().arg_types);
  auto rettype = inferer.unify(f1.getraw().ret_type, f2.getraw().ret_type);
  return types::Function{std::move(rettype), std::move(argtype)};
}

types::Value TypeUnifyVisitor::unify(types::rArray a1, types::rArray a2) {
  auto lhs = a1.getraw().elem_type;
  auto rhs = a2.getraw().elem_type;
  auto elemtype = inferer.unify(std::move(lhs), std::move(rhs));
  // size check?
  return types::Array{elemtype, a1.getraw().size};
}
types::Value TypeUnifyVisitor::unify(types::rStruct s1, types::rStruct s2) {
  // TODO(tomoya)
  return s1;
}
types::Value TypeUnifyVisitor::unify(types::rTuple f1, types::rTuple f2) {
  // TODO(tomoya)
  return f1;
}
std::vector<types::Value> TypeUnifyVisitor::unifyArgs(std::vector<types::Value>& v1,
                                                      std::vector<types::Value>& v2) {
  std::vector<types::Value> res;
  if (v1.size() != v2.size()) {
    throw std::runtime_error("type mismatch: argument size are different");
  }
  for (int i = 0; i < v1.size(); i++) {
    auto lhs = v1.at(i);
    auto rhs = v2.at(i);
    res.emplace_back(inferer.unify(lhs, rhs));  // discard return values
  }
  return res;
}

types::Value TypeInferer::inferStatements(ast::Statements& statements) {
  types::Value stmttype;
  for (auto&& statement : statements) { stmttype = std::visit(statementvisitor, *statement); }
  return stmttype;
}
TypeEnv& TypeInferer::infer(ast::Statements& topast) {
  inferStatements(topast);
  // typeenv.dumpTvLinks();
  substituteTypeVars();
  return typeenv;
}
void TypeInferer::substituteTypeVars() {
  for (auto&& [key, t] : typeenv.env) {
    auto [iter, replaced] = typeenv.env.insert_or_assign(key, std::visit(substitutevisitor, t));
  }
}

}  // namespace mimium
