#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/FileSystem.h"
#include "SPDPrinter.h"

#include <iostream>

using namespace llvm;

class IR2SPD : public FunctionPass {
public:
  static char ID;

  IR2SPD() : FunctionPass(ID) {
  }

  virtual bool runOnFunction(Function &F) override;
  virtual void getAnalysisUsage(AnalysisUsage &AU) const override;
};

bool IR2SPD::runOnFunction(Function &F) {
  std::string FunctionName = F.getName().str();
  if (FunctionName.compare("fpga_main") == 0) {
    std::error_code EC;
    raw_fd_ostream OS(FunctionName + ".spd", EC, sys::fs::F_None);
    if (EC) {
      std::cerr << "cannot create a output file";
      return false;
    }

    emitFunctionSPD(OS, F);
  }

  return false;
}

void IR2SPD::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

char IR2SPD::ID = 0;
static RegisterPass<IR2SPD> X("generate-spd", "generate SPD from LLVM IR",
                              false, false);
