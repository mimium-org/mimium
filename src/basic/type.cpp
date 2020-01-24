#include "basic/type.hpp"

namespace mimium {
namespace types {

Kind kindOf(const Value& v) { return std::visit(kindvisitor, v); }

bool isPrimitive(const Value& v) { return kindOf(v) == Kind::PRIMITIVE; }

std::string toString(const Value& v, bool verbose) {
  tostrvisitor.verbose = verbose;
  return std::visit(tostrvisitor, v);
}
void dump(const Value& v, bool verbose) {
  std::cerr << toString(v, verbose) << "\n";
}

Value getFunRettype(types::Value& v){
  return std::get<recursive_wrapper<Function>>(v).getraw().ret_type;
}
std::optional<Value> getNamedClosure(types::Value& v){
  // Closure* res;
  std::optional<Value> res=std::nullopt;
      if(auto ref = std::get_if<recursive_wrapper<Ref>>(&v)){
   if(auto alias = std::get_if<recursive_wrapper<Alias>>(&(ref->getraw().val))){
    Alias& a = *alias;
    if(auto cls = std::get_if<recursive_wrapper<Closure>>(&a.target)){
      res = *ref;
    }
   }
      }
  return res;
}


}  // namespace types
std::string TypeEnv::toString(bool verbose) {
  std::stringstream ss;
  types::ToStringVisitor vis;
  vis.verbose = verbose;
  for (auto& [key, val] : env) {
    ss << key << " : " << std::visit(vis, val) << "\n";
  }
  return ss.str();
}
void TypeEnv::dump(bool verbose){
std::cerr <<"-------------------\n"<< toString(verbose)<<"-------------------\n";
}
}  // namespace mimium