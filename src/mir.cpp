#include "mir.hpp"

namespace mimium {

std::string MIRblock::toString() {
  std::string str;
  for (int i = 0; i < indent_level; i++) {
    str += "  ";  // indent
  }
  str += label + ":\n";

  indent_level++;
  for (auto& inst : instructions) {
    for (int i = 0; i < indent_level; i++) {
      str += "  ";  // indent
    }

    str += std::visit([](auto val) -> std::string { return val->toString(); },
                      inst) +
           "\n";
  }
  indent_level--;
  return str;
}

bool MIRinstruction::isFreeVariable(std::shared_ptr<SymbolEnv> env,
                                    std::string str) {
  auto [isvarset, isfv] = env->isFreeVariable(str);  // check local vars

  return isfv;
}

bool MIRinstruction::gatherFV_raw(std::deque<TypedVal>& fvlist,
                                  std::shared_ptr<SymbolEnv> env,
                                  TypeEnv& typeenv, std::string& str,std::string& parent_name) {

  bool res = isFreeVariable(env, str);
  if (res) {
    auto it = std::find_if(fvlist.cbegin(),fvlist.cend(),[&str](const TypedVal& v){return v.name == str;});
    if(it == fvlist.cend()){ //duplication check
    fvlist.push_back({typeenv.find(str),str});
    }
    str = "fv_" + str + "_" + parent_name ;// destructive

  }
  return res;
}

void MIRinstruction::checkLvalue(std::deque<TypedVal>& fvlist,
                                 std::shared_ptr<ClosureConverter> cc,std::string& parent_name) {
  auto isvarset = cc->env->isVariableSet(lv_name);
  if (isvarset) {
    bool isfv = gatherFV_raw(fvlist, cc->env, cc->typeenv, lv_name,parent_name);
    if (!isfv) {
      cc->env->setVariableRaw(lv_name, "tmp");
    }
  } else {
    cc->env->setVariableRaw(lv_name, "tmp");
  }
}

std::string NumberInst::toString() {
  return lv_name + " = " + std::to_string(val);
}
void NumberInst::closureConvert(std::deque<TypedVal>& fvlist,
                                std::shared_ptr<ClosureConverter> cc,
                                std::shared_ptr<MIRblock> mir,
                                std::list<Instructions>::iterator it) {
  checkLvalue(fvlist, cc,mir->label);
}

std::string AllocaInst::toString() { return "alloca: "+lv_name+" (" +  std::visit([](auto t){return t.toString();},type) + ")"; }
void AllocaInst::closureConvert(std::deque<TypedVal>& fvlist,
                                std::shared_ptr<ClosureConverter> cc,
                                std::shared_ptr<MIRblock> mir,
                                std::list<Instructions>::iterator it) {
  checkLvalue(fvlist, cc,mir->label);
}

std::string RefInst::toString() { return lv_name + " = ref " + val; }
void RefInst::closureConvert(std::deque<TypedVal>& fvlist,
                             std::shared_ptr<ClosureConverter> cc,
                             std::shared_ptr<MIRblock> mir,
                             std::list<Instructions>::iterator it) {
  checkLvalue(fvlist, cc,mir->label);
  gatherFV_raw(fvlist, cc->env, cc->typeenv, val,mir->label);
}
std::string AssignInst::toString() { return lv_name + " =(overwrite) " + val; }
void AssignInst::closureConvert(std::deque<TypedVal>& fvlist,
                             std::shared_ptr<ClosureConverter> cc,
                             std::shared_ptr<MIRblock> mir,
                             std::list<Instructions>::iterator it) {
  checkLvalue(fvlist, cc,mir->label);
  gatherFV_raw(fvlist, cc->env, cc->typeenv, val,mir->label);
}


std::string TimeInst::toString() { return lv_name + " = " + val + +"@" + time; }
void TimeInst::closureConvert(std::deque<TypedVal>& fvlist,
                              std::shared_ptr<ClosureConverter> cc,
                              std::shared_ptr<MIRblock> mir,
                              std::list<Instructions>::iterator it) {
  gatherFV_raw(fvlist, cc->env, cc->typeenv, val,mir->label);
  gatherFV_raw(fvlist, cc->env, cc->typeenv, time ,mir->label);
  checkLvalue(fvlist, cc,mir->label);
}
std::string OpInst::toString() { return lv_name + " = " + lhs + op + rhs; }
void OpInst::closureConvert(std::deque<TypedVal>& fvlist,
                            std::shared_ptr<ClosureConverter> cc,
                            std::shared_ptr<MIRblock> mir,
                            std::list<Instructions>::iterator it) {
  gatherFV_raw(fvlist, cc->env, cc->typeenv, lhs ,mir->label);
  gatherFV_raw(fvlist, cc->env, cc->typeenv, rhs ,mir->label);
  checkLvalue(fvlist, cc,mir->label);
}
std::string FunInst::toString() {
  std::string s;
  s += lv_name + " = fun " + join(args, " , ");
  if (!freevariables.empty()) {
    s += " fv{ " + join(freevariables, " , ") + " }";
  }
  s += "\n";
  body->indent_level++;
  s += body->toString();
  body->indent_level--;

  return s;
}
void FunInst::closureConvert(std::deque<TypedVal>& fvlist,
                             std::shared_ptr<ClosureConverter> cc,
                             std::shared_ptr<MIRblock> mir,
                             std::list<Instructions>::iterator it) {
  auto tmpenv = cc->env;
  cc->env = cc->env->createNewChild(lv_name);
  for (auto& a : this->args) {
    cc->env->setVariableRaw(a, "arg");
  }
  for (auto cit = body->instructions.begin(), end = body->instructions.end();
       cit != end; cit++) {
    auto& childinst = *cit;
    std::visit(
        [&](auto c) {
          c->closureConvert(this->freevariables, cc, this->body, cit);
        },
        childinst);  // recursively visit;
  }
  if (this->freevariables.empty()) {
    cc->known_functions[lv_name] = shared_from_this();
  } else {
    auto fvtype = getFvType(this->freevariables);
    std::string newname =
        lv_name + "_cls";  //+ std::to_string(cc->capturecount++);
    auto makecls = std::make_shared<MakeClosureInst>(
        newname, lv_name, this->freevariables, fvtype);
    mir->instructions.insert(it, std::move(makecls));
    types::Function newtype =
        std::get<recursive_wrapper<types::Function>>(this->type);
    newtype.arg_types.emplace_back(fvtype);
    this->type = newtype;
    std::for_each(freevariables.begin(),freevariables.end(),[this](TypedVal& v){v.name = v.name+"_"+lv_name;});
  }
  cc->env = tmpenv;
  checkLvalue(fvlist, cc,mir->label);
  // post process?????
}

types::Struct FunInst::getFvType(std::deque<TypedVal>& fvlist) {
  std::vector<types::Value> v;
  for (auto& a : fvlist) {
    v.push_back(a.type);
  }
  return types::Struct(v);
}

void FunInst::moveFunToTop(std::shared_ptr<ClosureConverter> cc,
                           std::shared_ptr<MIRblock> mir,
                           std::list<Instructions>::iterator it) {
  auto& tinsts = cc->toplevel->instructions;
  auto cit = body->instructions.begin();
  while (cit != body->instructions.end()) {
    auto& childinst = *cit;
    std::visit(overloaded{[&](std::shared_ptr<FunInst> c) {
                            c->moveFunToTop(cc, this->body, cit);
                            cit++;
                          },
                          [&cit](auto c) { cit++; }},
               childinst);  // recursively visit;
  }
  if (cc->toplevel != mir) {
    tinsts.insert(tinsts.begin(), shared_from_this());
  }
  this->body->instructions.remove_if([](Instructions v) {
    return std::visit([](auto v) -> bool { return v->isFunction(); }, v);
  });
}

std::string MakeClosureInst::toString() {
  std::string s;
  s += lv_name + " = makeclosure " + fname + " " + join(captures, " , ");
  // s += body->toString();
  return s;
}
void MakeClosureInst::closureConvert(std::deque<TypedVal>& fvlist,
                                     std::shared_ptr<ClosureConverter> cc,
                                     std::shared_ptr<MIRblock> mir,
                                     std::list<Instructions>::iterator it) {
  // do nothing
}
std::string FcallInst::toString() {
  std::string s;
  return lv_name + " = app" + fcalltype_str[ftype] + " " + fname + " " +
         join(args, " , ");
}
void FcallInst::closureConvert(std::deque<TypedVal>& fvlist,
                               std::shared_ptr<ClosureConverter> cc,
                               std::shared_ptr<MIRblock> mir,
                               std::list<Instructions>::iterator it) {
  bool isknown = cc->known_functions.count(this->fname) > 0;
  if (isknown) {
    this->ftype = DIRECT;
  }
  for (auto& a : this->args) {
    gatherFV_raw(fvlist, cc->env, cc->typeenv, a ,mir->label);
  }
  checkLvalue(fvlist, cc,mir->label);
}

std::string ArrayInst::toString() {
  return lv_name + " = array " + name + " " + join(args, " , ");
}

void ArrayInst::closureConvert(std::deque<TypedVal>& fvlist,
                               std::shared_ptr<ClosureConverter> cc,
                               std::shared_ptr<MIRblock> mir,
                               std::list<Instructions>::iterator it) {
  for (auto& a : this->args) {
    gatherFV_raw(fvlist, cc->env, cc->typeenv, a ,mir->label);
  }
  checkLvalue(fvlist, cc,mir->label);
}

std::string ArrayAccessInst::toString() {
  return lv_name + " = arrayaccess " + name + " " + index;
}
void ArrayAccessInst::closureConvert(std::deque<TypedVal>& fvlist,
                                     std::shared_ptr<ClosureConverter> cc,
                                     std::shared_ptr<MIRblock> mir,
                                     std::list<Instructions>::iterator it) {
  gatherFV_raw(fvlist, cc->env, cc->typeenv, name ,mir->label);
  gatherFV_raw(fvlist, cc->env, cc->typeenv, index ,mir->label);
  checkLvalue(fvlist, cc,mir->label);
}

std::string IfInst::toString() {
  std::string s;
  s += lv_name + " = if " + cond + "\n";
  s += thenblock->toString();
  s += elseblock->toString();
  return s;
}
void IfInst::closureConvert(std::deque<TypedVal>& fvlist,
                            std::shared_ptr<ClosureConverter> cc,
                            std::shared_ptr<MIRblock> mir,
                            std::list<Instructions>::iterator it) {
  gatherFV_raw(fvlist, cc->env, cc->typeenv, cond ,mir->label);
  for (auto cit = thenblock->instructions.begin(),
            end = thenblock->instructions.end();
       cit != end; cit++) {
    auto& theninst = *cit;
    std::visit([&](auto c) { c->closureConvert(fvlist, cc, mir, cit); },
               theninst);
  }
  for (auto cit = elseblock->instructions.begin(),
            end = elseblock->instructions.end();
       cit != end; cit++) {
    auto& elseinst = *cit;
    std::visit([&](auto c) { c->closureConvert(fvlist, cc, mir, cit); },
               elseinst);
  }
}
std::string ReturnInst::toString() { return lv_name + " = return " + val; }
void ReturnInst::closureConvert(std::deque<TypedVal>& fvlist,
                                std::shared_ptr<ClosureConverter> cc,
                                std::shared_ptr<MIRblock> mir,
                                std::list<Instructions>::iterator it) {
  gatherFV_raw(fvlist, cc->env, cc->typeenv, val,mir->label);
  // std::visit( overloaded{
  //   [](types::Function f){},
  //   [](auto c){}
  // },this->type);
}

}  // namespace mimium