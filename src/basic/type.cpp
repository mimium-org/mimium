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


}  // namespace types
std::string TypeEnv::toString(bool verbose) {
  std::stringstream ss;
  types::ToStringVisitor vis;
  vis.verbose = true;
  for (auto& [key, val] : env) {
    ss << key << " : " << std::visit(vis, val) << "\n";
  }
  return ss.str();
}
void TypeEnv::dump(bool verbose){
std::cerr <<"-------------------\n"<< toString()<<"-------------------\n";
}
}  // namespace mimium