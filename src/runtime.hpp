#pragma once
#include <memory>

#include "alphaconvert_visitor.hpp"
#include "ast.hpp"
#include "closure_convert.hpp"
#include "driver.hpp"
#include "environment.hpp"
#include "knormalize_visitor.hpp"
#include "llvmgenerator.hpp"
#include "mididriver.hpp"
#include "scheduler.hpp"
#include "type_infer_visitor.hpp"

namespace mimium {
template<typename TaskType>
class Scheduler;

class Runtime : public std::enable_shared_from_this<Runtime> {
 public:
  Runtime(std::string filename_i = "untitled") : filename(filename_i) {}

  virtual ~Runtime() = default;

  virtual void addScheduler(bool issoundfile);
  virtual void init();
  void clear();
  inline void clearDriver() { driver.clear(); };
  void start();
  inline bool isrunning() { return running_status; };
  void stop();
  AST_Ptr loadSource(std::string src);
  virtual AST_Ptr loadSourceFile(std::string filename);
  Mididriver& getMidiInstance() { return midi; };

  AST_Ptr getMainAst() { return driver.getMainAst(); };
  auto getScheduler() { return sch; };

  void setWorkingDirectory(const std::string path) {
    current_working_directory = path;
    driver.setWorkingDirectory(path);
  }
  std::string current_working_directory = "";

 protected:
  std::shared_ptr<Scheduler<AST_Ptr>> sch;
  mmmpsr::MimiumDriver driver;
  Mididriver midi;
  std::string filename;
  bool running_status = false;
};

class Runtime_LLVM : public Runtime {
 public:
  explicit Runtime_LLVM(std::string filename = "untitled.mmm" ,bool isjit=true);

  ~Runtime_LLVM() = default;
  void addScheduler(bool issoundfile) override;
  AST_Ptr alphaConvert(AST_Ptr _ast);
  TypeEnv& typeInfer(AST_Ptr _ast);
  TypeEnv& getTypeEnv() { return typevisitor.getEnv(); };
  std::shared_ptr<MIRblock> kNormalize(AST_Ptr _ast);
  std::shared_ptr<MIRblock> closureConvert(std::shared_ptr<MIRblock> mir);
  auto llvmGenarate(std::shared_ptr<MIRblock> mir) -> std::string;
  int execute();
  AST_Ptr loadSourceFile(std::string filename) override;
  // virtual void loadAst(AST_Ptr _ast) override;

 private:
  AlphaConvertVisitor alphavisitor;
  TypeInferVisitor typevisitor;
  std::shared_ptr<TypeInferVisitor> ti_ptr;
  KNormalizeVisitor knormvisitor;
  std::shared_ptr<ClosureConverter>closureconverter;
  std::shared_ptr<LLVMGenerator> llvmgenerator;
};
}  // namespace mimium