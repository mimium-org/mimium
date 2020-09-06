/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once
#include "basic/mir.hpp"
#include "basic/variant_visitor_helper.hpp"
#include "compiler/ffi.hpp"
namespace mimium {

class ClosureConverter : public std::enable_shared_from_this<ClosureConverter> {
 public:
  explicit ClosureConverter(TypeEnv& typeenv);
  ~ClosureConverter();
  mir::blockptr convert(mir::blockptr toplevel);
  void reset();
  bool hasCapture(const std::string& fname) { return fvinfo.count(fname) > 0; }

  auto& getCaptureNames(const std::string& fname) { return fvinfo[fname]; }
  auto& getCaptureType(const std::string& fname) { return clstypeenv[fname]; }
  void dump();

 private:
  TypeEnv& typeenv;
  mir::blockptr toplevel;
  int capturecount;
  int closurecount;
  std::unordered_map<std::string, int> known_functions;
  std::unordered_map<std::string, std::vector<std::string>> fvinfo;
  // fname: types::Tuple(...)
  std::unordered_map<std::string, types::Value> clstypeenv;

  void moveFunToTop(mir::blockptr mir);
  bool isKnownFunction(const std::string& name);
  std::string makeCaptureName() {
    return "Capture." + std::to_string(capturecount++);
  }
  std::string makeClosureTypeName() {
    return "Closure." + std::to_string(closurecount++);
  }

  struct CCVisitor {
    explicit CCVisitor(ClosureConverter& cc, std::vector<std::string>& fvlist,
                       std::vector<std::string>& localvlist,
                       std::list<mir::Instructions>::iterator& position)
        : cc(cc), fvlist(fvlist), localvlist(localvlist), position(position) {}

    ClosureConverter& cc;
    std::vector<std::string>& fvlist;
    std::vector<std::string>& localvlist;
    std::list<mir::Instructions>::iterator position;
    void updatepos() { ++position; }
    void registerFv(std::string& name);

    void operator()(mir::RefInst& i);
    void operator()(mir::AssignInst& i);
    void operator()(mir::OpInst& i);
    void operator()(mir::FunInst& i);
    void operator()(mir::FcallInst& i);
    void operator()(mir::MakeClosureInst& i);
    void operator()(mir::ArrayInst& i);
    void operator()(mir::ArrayAccessInst& i);
    void operator()(mir::IfInst& i);
    void operator()(mir::ReturnInst& i);
    template <typename T>
    void operator()(T& i) {
      if constexpr (std::is_base_of_v<mir::instruction, std::decay_t<T>>) {
        localvlist.push_back(i.lv_name);
      } else {
        static_assert(true, "mir instruction unreachable");
      }
    }
    bool isFreeVar(const std::string& name);

   private:
    void visitinsts(mir::FunInst& i, CCVisitor& ccvis,
                    std::list<mir::Instructions>::iterator pos);
  };
  friend struct CCVisitor;
};
}  // namespace mimium