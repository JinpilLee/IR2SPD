#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopIterator.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Constants.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <iostream>

using namespace llvm;

class IR2SPD : public FunctionPass {
public:
  static char ID;

  IR2SPD() : FunctionPass(ID) {
  }

  virtual bool runOnFunction(Function &F);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
};

bool IR2SPD::runOnFunction(Function &F) {
  std::cerr << "##### Module --------------------\n";

  // print name
  std::cerr << "Name\t\t" << F.getName().str() << ";\n";

  // print input ports
  FunctionType *FT = F.getFunctionType();
  unsigned NumParams = FT->getNumParams();
  if (NumParams != 0) {
    std::cerr << "Main_In\t\t{main_i::";
    unsigned Idx = 0;
    for (const Argument &A : F.args()) {
      if (Idx != 0) std::cerr << ",";
      Idx++;
      std::cerr << "in_" << A.getName().str();
    }
    std::cerr << "};\n";
  }

  // print output port
  std::cerr << "Main_Out\t{main_o::out_0};\n";

  // print param

  // print statement
  for (BasicBlock &ibb : F)
    for (BasicBlock::iterator iib = ibb.begin(), iie = ibb.end(); iib != iie;
         ++iib) {
    }
  }

  return false;
}

void IR2SPD::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

char IR2SPD::ID = 0;
static RegisterPass<IR2SPD> X("generate-spd", "generate SPD from LLVM IR",
                              false, false);
