#pragma once
#include "helper_functions.hpp"
namespace mimium {
struct Pos {
  int line = 1;
  int col;
};
inline std::string toString(Pos const& p) {
  return "L" + std::to_string(p.line) + ":C" + std::to_string(p.col);
}
struct SourceLoc {
  Pos begin;
  Pos end;
};

inline std::string toString(SourceLoc const& loc) {
  return toString(loc.begin) + " ~ " + toString(loc.end);
}

inline std::ostream& operator<<(std::ostream& os, const SourceLoc& loc) {
  os << toString(loc);
  return os;
}

struct DebugInfo {
  SourceLoc source_loc;
  std::string symbol;
};
}  // namespace mimium