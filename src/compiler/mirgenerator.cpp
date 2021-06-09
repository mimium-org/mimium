/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "compiler/mirgenerator.hpp"
#include "compiler/type_lowering.hpp"

namespace mimium {

namespace minst = mir::instruction;

namespace {

auto makeMirVal = [](auto&& a) {
  return std::make_shared<mir::Value>(std::forward<decltype(a)>(a));
};

auto emplace = [](const auto& inst, mir::blockptr block) {
  return mir::addInstToBlock(mir::Instructions{std::move(inst)}, block);
};

}  // namespace

template <class... Ts>
struct MirGenerator : Ts... {
  using Ts::operator()...;
  int varcounter = 0;
  std::string makeNewName() { return "k" + std::to_string(varcounter++); }
  std::optional<std::string> lvar_holder;
  std::string getOrMakeName() {
    auto res = lvar_holder.value_or(this->makeNewName());
    if (lvar_holder) { lvar_holder = std::nullopt; }
    return res;
  }

  std::shared_ptr<Environment<std::string, mir::valueptr>> symbol_env;
  mir::valueptr self_holder;
  //

  mir::valueptr makeTuple(List<mir::valueptr> const& values, const TypeEnvH& typeenv,
                          mir::blockptr block) {
    auto lvname = this->makeNewName();
    auto tlist = fmap(values, [](mir::valueptr v) { return Box(mir::getType(*v)); });
    auto tupletype = LType::Value{LType::Tuple{tlist}};
    mir::valueptr lvar =
        emplace(minst::Allocate{{lvname, LType::Value{LType::Pointer{tupletype}}}}, block);
    int count = 0;
    auto type_iter = tlist.cbegin();
    for (auto const& elem : values) {
      auto newlvname = this->makeNewName();
      auto index = std::make_shared<mir::Value>(mir::Constants{count});
      auto ptrtostore =
          emplace(minst::Field{{newlvname, *type_iter}, lvar, std::move(index)}, block);
      emplace(minst::Store{{newlvname, *type_iter}, ptrtostore, elem}, block);
      count++;
      type_iter++;
    }
    return lvar;
  }
  mir::valueptr generateInst(const LAst::expr& expr, const TypeEnvH& typeenv, mir::blockptr block,
                             mir::valueptr fnctx) {
    auto&& genmir = [&](auto&& a) { return generateInst(a, typeenv, block, fnctx); };
    auto&& pushblock = [&](auto lvname, mir::valueptr parent, auto&& action) {
      auto newblock = mir::makeBlock(lvname, block->indent_level + 1);
      newblock->parent = parent;
      auto res = action(newblock);
      return res;
    };
    auto&& vis = overloaded{
        [&](LAst::FloatLit const& a) { return makeMirVal(mir::Constants{a.v}); },
        [&](LAst::IntLit const& a) { return makeMirVal(mir::Constants{a.v}); },
        [&](LAst::BoolLit const& a) { return makeMirVal(mir::Constants{a.v}); },
        [&](LAst::StringLit const& a) { return makeMirVal(mir::Constants{a.v}); },
        [&](LAst::TupleLit const& a) { return makeTuple(fmap(a.v, genmir), typeenv, block); },
        [&](LAst::TupleGet const& a) {
          auto newname = makeNewName();
          auto index = a.v.field;
          auto expr = genmir(a.v.expr);
          auto type = mir::getType(*expr);
          auto tuptype = std::get<LType::Tuple>(type.v);
          auto valtype = *std::next(tuptype.v.cbegin(), index);
          return emplace(minst::Field{{newname, valtype}, expr, makeMirVal(mir::Constants{index})},
                         block);
        },
        [&](LAst::StructLit const& a) {
          return makeTuple(fmap(a.v, [&](LAst::StructKey const& a) { return genmir(a.v); }),
                           typeenv, block);
        },
        [&](LAst::StructGet const& a) {
          auto newname = makeNewName();
          auto key = a.v.field;
          auto expr = genmir(a.v.expr);
          auto type = mir::getType(*expr);
          auto sttype = std::get<LType::Record>(type.v);
          auto [valtype, index] = getRecordTypeByKey(sttype, key);
          return emplace(minst::Field{{newname, valtype}, expr, makeMirVal(mir::Constants{index})},
                         block);
        },
        [&](LAst::ArrayLit const& a) {
          auto newname = makeNewName();
          auto newelems = fmap(a.v, genmir);
          mir::valueptr firstelem = *newelems.cbegin();
          auto type = LType::Array{mir::getType(*firstelem), static_cast<int>(newelems.size())};
          return emplace(minst::Array{{newname, LType::Value{type}}, newelems}, block);
        },
        [&](LAst::ArrayGet const& a) {
          auto array = genmir(a.v.expr);
          auto index = genmir(a.v.field);
          LType::Value const& type = mir::getType(*array);
          LType::Value const& vtype = std::get<LType::Pointer>(type.v).v;
          LType::Value rettype;
          if (std::holds_alternative<LType::Array>(vtype.v)) {
            rettype.v = std::get<LType::Array>(vtype.v);
          } else if (std::holds_alternative<LType::Float>(vtype.v)) {
            rettype.v = vtype;
          } else {
            throw std::runtime_error("[] operator cannot be used for other than array type");
          }
          auto newname = makeNewName();
          return emplace(minst::ArrayAccess{{newname, rettype}, array, index}, block);
        },
        [&](LAst::ArraySize const& a) -> mir::valueptr {
          // TODO
          return nullptr;
        },
        [&](LAst::Symbol const& a) -> mir::valueptr {
          if (a.v == mir::getName(*fnctx)) {  // recursive call
            return fnctx;
          }
          auto opt_res = symbol_env->search(a.v);
          if (opt_res) { return opt_res.value(); }
          auto ext_res_iter = Intrinsic::ftable.find(a.v);
          if (ext_res_iter == Intrinsic::ftable.cend()) {
            auto fn = ext_res_iter->second;
            return makeMirVal(
                mir::ExternalSymbol{std::string(fn.target_fnname), lowerType(fn.mmmtype)});
          }
          return nullptr;
        },
        [&](LAst::SelfLit const& a) {
          if (fnctx == nullptr) {
            throw std::runtime_error("\"self\" cannot be used in global context.");
          }
          auto fntype = mir::getType(*fnctx);
          auto rettype = std::get<LType::Function>(fntype.v).v.second;
          return makeMirVal(mir::Self{fnctx});
        },
        [&](LAst::Lambda const& a) {
          auto lvname = getOrMakeName();
          auto env = *typeenv.child_env.value();  // TODO error handling
          symbol_env = symbol_env->expand();
          auto fun = minst::Function{
              {lvname, LType::Value{LType::Unit{}}},
              mir::FnArgs{std::nullopt, fmap(a.v.args, [&](auto const& arg) {
                            auto type = env.search(arg.v).value();
                            auto a = mir::Argument{arg.v, lowerType(type), mir::valueptr(nullptr)};
                            auto res = std::make_shared<mir::Argument>(std::move(a));
                            symbol_env->addToMap(arg.v, makeMirVal(res));
                            return res;
                          })}};
          auto resptr = emplace(fun, block);
          auto body = pushblock(lvname, resptr,
                                [&](auto&& b) { return generateInst(a.v.body, env, b, resptr); });

          auto& resptr_fref = mir::getInstRef<minst::Function>(resptr);
          auto argtype = fmap(
              a.v.args, [&](auto const& a) { return Box(lowerType(env.search(a.v).value())); });
          LType::Value rettype = mir::getType(*body);
          resptr_fref.type = LType::Value{LType::Function{std::pair(argtype, Box(rettype))}};
          if (!std::holds_alternative<LType::Unit>(rettype.v) && isAggregate<LType>(rettype)) {
            auto fref = resptr_fref;
            auto& retval = resptr;
            auto loadinst = mir::addInstToBlock(minst::Load{{lvname + "_res", rettype}, retval},
                                                resptr_fref.body);
            fref.args.ret_ptr = std::make_shared<mir::Argument>(
                mir::Argument{lvname + "_retptr", LType::Value{LType::Pointer{rettype}}, resptr});
            fref.body->instructions.pop_back();
            mir::addInstToBlock(
                minst::Store{{"store", LType::Value{LType::Unit{}}},
                             std::make_shared<mir::Value>(fref.args.ret_ptr.value()),
                             loadinst},
                fref.body);
          }
          symbol_env = symbol_env->parent_env.value();
          return resptr;
        },
        [&](LAst::Sequence const& a) {
          auto first = genmir(a.v.first);
          return genmir(a.v.second);
        },
        [&](LAst::NoOp const& a) -> mir::valueptr {
          assert(false);
          return nullptr;
        },
        [&](LAst::Let const& a) {
          auto lv = genmir(a.v.expr);
          symbol_env->addToMap(a.v.id.v, lv);
          lvar_holder = a.v.id.v;
          auto env = *typeenv.child_env.value();  // TODO error handling
          return generateInst(a.v.body, env, block, fnctx);
        },
        [&](LAst::LetTuple const& a) {
          auto lv = genmir(a.v.expr);
          auto env = *typeenv.child_env.value();  // TODO error handling
          symbol_env = symbol_env->expand();
          for (auto& args : a.v.id) {
            // allocate
            auto newname = makeNewName();
            auto type = env.search(args.v);
            auto vtype = lowerType(type.value());
            auto ptype = LType::Value{LType::Pointer{vtype}};
            mir::valueptr lvarptr = emplace(minst::Allocate{{args.v + "_ptr", ptype}}, block);

            mir::valueptr field = emplace(minst::Field{{args.v + "_ptr", ptype}}, block);
            mir::valueptr lvar = emplace(minst::Load{{args.v, vtype}, field}, block);
            symbol_env->addToMap(args.v, lvar);
          }
          auto res = generateInst(a.v.body, env, block, fnctx);
          symbol_env = symbol_env->parent_env.value();
          return res;
        },
        [&](LAst::App const& a) {
          auto newname = makeNewName();
          auto callee = genmir(a.v.callee);
          auto fntype = mir::getType(*callee);
          auto rettype = std::get<LType::Function>(fntype.v).v.second;
          auto args = fmap(a.v.args, genmir);
          return emplace(minst::Fcall{{newname, rettype}, callee, args}, block);
        },
        [&](LAst::If const& a) {
          auto gen_if_block = [&](auto&& a, std::string const& name) {
            auto newblock = mir::makeBlock(name, block->indent_level + 1);
            newblock->parent = fnctx;
            auto res = generateInst(a, typeenv, newblock, fnctx);
            return std::pair(newblock, res);
          };
          auto lvname = makeNewName();
          auto cond = genmir(a.v.cond);
          auto [thenblock, thenres] = gen_if_block(a.v.vthen, lvname + "$then");

          auto opt_elseblock =
              fmap(a.v.velse, [&](auto&& a) { return gen_if_block(a, lvname + "$else").first; });
          return emplace(
              minst::If{{lvname, mir::getType(*thenres)}, cond, thenblock, opt_elseblock}, block);
        }};
    return std::visit(vis, expr.v);
  }
};

template <class... Ts>
MirGenerator(Ts&&... ts) -> MirGenerator<Ts...>;

// valueptrを生成しながら副作用としてblockを生成しているわけで、、、
// blockを引数に受け取るgenrateinstructionみたいなのを作るのがいいのか

mir::blockptr generateMir(const LAst::expr& expr, const TypeEnvH& typeenv) {
  auto main = mir::makeBlock("main");
  MirGenerator<> mirgen;
  mirgen.generateInst(expr, typeenv, main, nullptr);
  return main;
}

}  // namespace mimium