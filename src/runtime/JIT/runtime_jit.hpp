#pragma once
#include "runtime/runtime.hpp"
#include "runtime/JIT/jit_engine.hpp"

namespace mimium{
    struct LLVMTaskType {
  void* addresstofn;
  //int64_t tasktypeid;
  double arg;
  void* addresstocls;
};
 class Runtime_LLVM : public Runtime<LLVMTaskType> , public std::enable_shared_from_this<Runtime_LLVM>{
 public:
  explicit Runtime_LLVM(std::string filename = "untitled.mmm",
                        bool isjit = true);

  ~Runtime_LLVM() = default;
void addScheduler(bool is_soundfile)override;
  void start() override;
  DspFnType getDspFn() override;
  void* getDspFnCls()override;

  void executeModule(std::unique_ptr<llvm::Module> module);
  auto& getJitEngine(){return *jitengine;}
  llvm::LLVMContext& getLLVMContext(){return jitengine->getContext();}
 void addAudioDriver(std::shared_ptr<AudioDriver> a)override;


 private:

  DspFnType dspfn_address = nullptr;
  void* dspfn_cls_address=nullptr;
  std::unique_ptr<llvm::orc::MimiumJIT> jitengine;

};   
}