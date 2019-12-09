#include "environment.hpp"
namespace mimium {

void InterpreterEnv::setVariable(std::string key, mValue val){
    mValue newval = 0.0;
    if (auto pval = std::get_if<std::string>(&val)) {
      newval = findVariable(*pval);
    } else {
      newval = val;
    }
    if (!variables.empty() && variables.count(key) > 0) {  // search dictionary
      Logger::debug_log("overwritten", Logger::DEBUG);
      variables.insert_or_assign(key, newval);  // overwrite exsisting value

    } else if (parent != nullptr) {
      parent->setVariable(key, newval);  // search recursively
    } else {
      Logger::debug_log("Create New Variable " + key, Logger::DEBUG);
      variables.emplace(key, newval);
    }
  };




}  // namespace mimium
