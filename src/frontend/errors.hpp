#pragma once
#include "basic/error_def.hpp"

namespace mimium {
struct ApplicationError : public Error {
  explicit ApplicationError(const std::string& what) : Error(what) {}
};

struct CliAppError : public ApplicationError {
  explicit CliAppError(const std::string& what) : ApplicationError(what) {}
};

}  // namespace mimium