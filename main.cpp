#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "SPDPrinter.h"

using namespace llvm;

class IR2SPD : public CallGraphSCCPass {
public:
  static char ID;

  IR2SPD() : CallGraphSCCPass(ID) {
  }

  virtual bool runOnSCC(CallGraphSCC &SCC) override;
  virtual void getAnalysisUsage(AnalysisUsage &AU) const override;

private:
  SPDModuleMapTy SPDModuleMap;

  unsigned calcModuleDelay(CallGraphNode *N) const;
  void emitSPDRec(CallGraphNode *N);
};

bool IR2SPD::runOnSCC(CallGraphSCC &SCC) {
  for (CallGraphNode *Node : SCC) {
    Function *F = Node->getFunction();
    if (!F) continue;

    if (isEntryFunction(*F)) {
      emitSPDRec(Node);
    }
  }

  return false;
}

// FIXME this is temporary function for test
// the delay value should be calculated by using the SPD compiler
unsigned IR2SPD::calcModuleDelay(CallGraphNode *N) const {
  std::string FuncName = N->getFunction()->getName().str();
  SPDModuleMapTy::const_iterator Iter = SPDModuleMap.find(FuncName);
  if (Iter != SPDModuleMap.end()) {
    return Iter->second;
  }

  // assume that the default delay is 5
  unsigned RetDelay = 5;
  for (const auto &I : *N) {
    CallGraphNode *CalledFuncNode = I.second;
    Function *CalledFunc = CalledFuncNode->getFunction();
    if (CalledFunc) {
      // assume that the function call delay is 10
      unsigned TempDelay = calcModuleDelay(CalledFuncNode) + 10;
      if (TempDelay > RetDelay) {
        RetDelay = TempDelay;
      }
    }
  }

  return RetDelay;
}

void IR2SPD::emitSPDRec(CallGraphNode *N) {
  Function *Func = N->getFunction();
  std::string FuncName = Func->getName().str();
  SPDModuleMapTy::iterator Iter = SPDModuleMap.find(FuncName);
  if (Iter != SPDModuleMap.end()) {
    return;
  }

  for (const auto &I : *N) {
    CallGraphNode *CalledFuncNode = I.second;
    Function *CalledFunc = CalledFuncNode->getFunction();
    if (CalledFunc) {
      emitSPDRec(CalledFuncNode);
    }
  }

  // FIXME there are two ways to calculate the delay
  // 1. generate SPD file and invoke the compiler
  // 2. generate in-memory SPD instance and use the compiler API
  // current implementation assumes method 1
  emitFunctionSPD(*Func, SPDModuleMap);
  // FIXME invoke SPD compiler here to calculate the delay
  // instead of using calcModuleDelay()
  unsigned FuncDelay = calcModuleDelay(N);
  SPDModuleMap[FuncName] = FuncDelay;
}

void IR2SPD::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

char IR2SPD::ID = 0;
static RegisterPass<IR2SPD> X("generate-spd", "generates SPD from LLVM IR",
                              false, false);
