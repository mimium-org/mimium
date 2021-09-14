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
  MirGenerator() : symbol_env(std::make_shared<Environment<std::string, mir::valueptr>>()) {}
  int varcounter = 0;
  std::string makeNewName() { return "k" + std::to_string(varcounter++); }
  std::optional<std::string> lvar_holder;
  std::string getOrMakeName() {
    if (lvar_holder.has_value()) {
      auto tmp = lvar_holder.value();
      lvar_holder = std::nullopt;
      return tmp;
    }
    return this->makeNewName();
  }

  std::shared_ptr<Environment<std::string, mir::valueptr>> symbol_env;
  mir::valueptr self_holder;
  //

  mir::valueptr makeTuple(List<mir::valueptr> const& values, const TypeEnvH& typeenv,
                          mir::blockptr block) {
    auto lvname = this->getOrMakeName();
    auto tlist = fmap(values, [](mir::valueptr v) { return Box(mir::getType(*v)); });
    auto tupletype = LType::Value{LType::Tuple{tlist}};
    mir::valueptr lvar =
        emplace(minst::Allocate{{lvname, LType::Value{LType::Pointer{tupletype}}}}, block);
    int count = 0;
    auto type_iter = tlist.cbegin();
    for (auto const& elem : values) {
      auto newlvname = this->getOrMakeName();
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
    const bool isglobal = fnctx == nullptr;
    auto&& genmir = [&](auto&& a) { return generateInst(a, typeenv, block, fnctx); };

    auto&& vis = overloaded{
        [&](LAst::FloatLit const& a) { return makeMirVal(mir::Constants{a.v}); },
        [&](LAst::IntLit const& a) { return makeMirVal(mir::Constants{a.v}); },
        [&](LAst::BoolLit const& a) { return makeMirVal(mir::Constants{a.v}); },
        [&](LAst::StringLit const& a) { return makeMirVal(mir::Constants{a.v}); },
        [&](LAst::TupleLit const& a) { return makeTuple(fmap(a.v, genmir), typeenv, block); },
        [&](LAst::TupleGet const& a) {
          auto newname = getOrMakeName();
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
          auto newname = getOrMakeName();
          auto key = a.v.field;
          auto expr = genmir(a.v.expr);
          auto type = mir::getType(*expr);
          auto sttype = std::get<LType::Record>(type.v);
          auto [valtype, index] = getRecordTypeByKey(sttype, key);
          return emplace(minst::Field{{newname, valtype}, expr, makeMirVal(mir::Constants{index})},
                         block);
        },
        [&](LAst::ArrayLit const& a) {
          auto newname = getOrMakeName();
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
          auto newname = getOrMakeName();
          return emplace(minst::ArrayAccess{{newname, rettype}, array, index}, block);
        },
        [&](LAst::ArraySize const& a) -> mir::valueptr {
          // TODO
          return nullptr;
        },
        [&](LAst::Symbol const& a) -> mir::valueptr {
          if (!isglobal && a.v.getUniqueName() == mir::getName(*fnctx)) {  // recursive call
            return fnctx;
          }
          auto opt_res = symbol_env->search(a.v.getUniqueName());
          if (opt_res) {
            auto ptrv = opt_res.value();
            auto ptrt = mir::getType(*ptrv).v;
            if (std::holds_alternative<LType::Pointer>(ptrt)) {
              auto vtype = std::get<LType::Pointer>(ptrt).v;
              if (isAggregate<LType>(vtype)) { return ptrv; }
              auto res = emplace(minst::Load{{a.v.getUniqueName(), vtype}, opt_res.value()}, block);
              return res;
            }
            return opt_res.value();
          }
          auto ext_res_iter = Intrinsic::ftable.find(a.v.v);
          if (ext_res_iter != Intrinsic::ftable.cend()) {
            auto fn = ext_res_iter->second;
            return makeMirVal(mir::ExternalSymbol{std::string(a.v.v), lowerType(fn.mmmtype)});
          }
          return nullptr;
        },
        [&](LAst::SelfLit const& a) {
          if (isglobal) { throw std::runtime_error("\"self\" cannot be used in global context."); }
          auto fntype = mir::getType(*fnctx);
          auto rettype = std::get<LType::Function>(fntype.v).v.second;
          return makeMirVal(mir::Self{fnctx});
        },
        [&](LAst::Lambda const& a) {
          auto lvname = getOrMakeName();

          auto fun = minst::Function{
              {lvname, LType::Value{LType::Unit{}}},
              mir::FnArgs{std::nullopt, fmap(a.v.args, [&](LAst::Lvar const& arg) {
                            auto type = *typeenv.search(arg.id.getUniqueName()).value();
                            auto a = mir::Argument{arg.id.getUniqueName(), lowerType(type),
                                                   mir::valueptr(nullptr)};
                            auto res = std::make_shared<mir::Argument>(std::move(a));
                            symbol_env->addToMap(arg.id.getUniqueName(), makeMirVal(res));
                            return res;
                          })}};
          auto resptr = emplace(fun, block);

          auto newblock = mir::makeBlock(lvname, block->indent_level + 1);
          newblock->parent = resptr;
          auto bodyret = generateInst(a.v.body, typeenv, newblock, resptr);
          auto& resptr_fref = mir::getInstRef<minst::Function>(resptr);
          for (auto& a : resptr_fref.args.args) { a->parentfn = resptr; }
          resptr_fref.body = newblock;
          auto argtype = fmap(a.v.args, [&](LAst::Lvar const& a) -> Box<LType::Value> {
            return Box(lowerType(*typeenv.search(a.id.getUniqueName()).value()));
          });
          LType::Value rettype = mir::getType(*bodyret);
          const bool ispassbyref =
              !std::holds_alternative<LType::Unit>(rettype.v) && isAggregate<LType>(rettype);

          if (!ispassbyref) {
            resptr_fref.type = LType::Value{LType::Function{std::pair(argtype, Box(rettype))}};
          }
          if (ispassbyref) {
            if (!std::holds_alternative<LType::Pointer>(rettype.v)) {  // workaround
              rettype = LType::Value{LType::Pointer{rettype}};
            }
            argtype.push_front(rettype);
            resptr_fref.type =
                LType::Value{LType::Function{std::pair(argtype, LType::Value{LType::Unit{}})}};

            auto loadinst = mir::addInstToBlock(minst::Load{{lvname + "_res", rettype}, bodyret},
                                                resptr_fref.body);
            resptr_fref.args.ret_ptr =
                std::make_shared<mir::Argument>(mir::Argument{lvname + "_retptr", rettype, resptr});
            mir::addInstToBlock(
                minst::Store{{"store", LType::Value{LType::Unit{}}},
                             std::make_shared<mir::Value>(resptr_fref.args.ret_ptr.value()),
                             loadinst},
                resptr_fref.body);
          } else if (!std::holds_alternative<LType::Unit>(rettype.v)) {
            mir::addInstToBlock(minst::Return{{"ret_" + mir::getName(*bodyret), rettype}, bodyret},
                                resptr_fref.body);
          }
          return resptr;
        },
        [&](LAst::Sequence const& a) {
          auto first = genmir(a.v.first);
          return genmir(a.v.second);
        },
        [&](LAst::Store const& a) {
          auto ptr = symbol_env->search(a.id.getUniqueName());
          assert(ptr.has_value());
          auto stored_v = genmir(a.expr);
          return emplace(
              minst::Store{{"store", LType::Value{LType::Unit{}}}, ptr.value(), stored_v}, block);
        },
        [&](LAst::NoOp const& a) -> mir::valueptr {
          assert(false);
          return nullptr;
        },
        [&](LAst::Let const& a) -> mir::valueptr {
          const auto id = a.v.id.id.getUniqueName();
          lvar_holder = id;
          auto val_to_store = genmir(a.v.expr);
          if (std::holds_alternative<LAst::NoOp>(a.v.body.getraw().v)) {
            return emplace(minst::NoOp{{"nop", LType::Value{LType::Unit{}}}}, block);
          }

          auto newname = getOrMakeName();

          auto vtype = typeenv.search(id);
          auto lvtype = lowerType(*vtype.value());
          if (!isAggregate<LType>(lvtype)) {
            auto ptype = LType::Value{LType::Pointer{lvtype}};
            mir::valueptr lvarptr = emplace(minst::Allocate{{id + "_ptr", ptype}}, block);
            mir::valueptr lvar = emplace(
                minst::Store{{"store", LType::Value{LType::Unit{}}}, lvarptr, val_to_store}, block);

            mir::valueptr res = emplace(minst::Load{{id, lvtype}, lvarptr}, block);
            symbol_env->addToMap(id, lvarptr);
          } else {
            symbol_env->addToMap(id, val_to_store);
          }
          return generateInst(a.v.body, typeenv, block, fnctx);
        },
        [&](LAst::LetTuple const& a) {
          auto lv = genmir(a.v.expr);
          int idx = 0;
          for (const auto& args : a.v.id) {
            // allocate
            auto newname = getOrMakeName();
            auto vtype = typeenv.search(args.id.getUniqueName());
            auto lvtype = lowerType(*vtype.value());
            auto ptype = LType::Value{LType::Pointer{lvtype}};
            mir::valueptr field =
                emplace(minst::Field{{args.id.getUniqueName() + "_ptr", ptype},
                                     lv,
                                     std::make_shared<mir::Value>(mir::Constants{idx++})},
                        block);
            symbol_env->addToMap(args.id.getUniqueName(), field);
          }
          auto res = generateInst(a.v.body, typeenv, block, fnctx);
          return res;
        },
        [&](LAst::App const& a) {
          auto newname = getOrMakeName();
          const auto callee = genmir(a.v.callee);
          const bool isrecursive = callee == fnctx;
          auto calltype = mir::isExternalSymbol(*callee) ? FCALLTYPE::EXTERNAL : FCALLTYPE::CLOSURE;
          LType::Value fntype;
          if (isrecursive) {
            mir::getInstRef<minst::Function>(fnctx).isrecursive = true;
            auto fname = mir::getName(*callee);
            auto ftype_opt = typeenv.search(fname);
            assert(ftype_opt.has_value());
            fntype = lowerType(*ftype_opt.value());
          } else {
            fntype = mir::getType(*callee);
          }
          auto& fntype_f = std::get<LType::Function>(fntype.v);
          LType::Value rettype = fntype_f.v.second;
          auto args = fmap(a.v.args, genmir);
          if (args.size() == fntype_f.v.first.size() - 1) {
            rettype = fntype_f.v.first.front();
            assert(std::holds_alternative<LType::Pointer>(rettype.v));
            auto resptr = emplace(minst::Allocate{{newname + "_ret", rettype}}, block);
            args.push_front(resptr);
            emplace(minst::Fcall{{makeNewName(), rettype}, callee, args, calltype}, block);
            return resptr;
          }
          return emplace(minst::Fcall{{newname, rettype}, callee, args, calltype}, block);
        },
        [&](LAst::If const& a) {
          auto gen_if_block = [&](auto&& a, std::string const& name) {
            auto newblock = mir::makeBlock(name, block->indent_level + 1);
            newblock->parent = fnctx;
            auto res = generateInst(a, typeenv, newblock, fnctx);
            auto rettype = mir::getType(*res);
            if (std::holds_alternative<LType::Unit>(rettype.v)) { return std::pair(newblock, res); }
            auto ret = mir::addInstToBlock(minst::Return{{"ret_" + name, mir::getType(*res)}, res},
                                           newblock);
            return std::pair(newblock, ret);
          };
          auto lvname = getOrMakeName();
          auto cond = genmir(a.v.cond);
          auto [thenblock, thenres] = gen_if_block(a.v.vthen, lvname + "$then");

          auto opt_elseblock =
              fmap(a.v.velse, [&](auto&& a) { return gen_if_block(a, lvname + "$else").first; });
          auto ltype = mir::getType(*thenres);
          return emplace(minst::If{{lvname, ltype}, cond, thenblock, opt_elseblock}, block);
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