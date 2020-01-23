#include "basic/type.hpp"

namespace mimium::types {

Kind kindOf(const Value& v) { return std::visit(kindvisitor, v); }

bool isPrimitive(const Value& v) { return kindOf(v) == Kind::PRIMITIVE; }

std::string toString(const Value& v){
  return std::visit(tostrvisitor,v);
}
}