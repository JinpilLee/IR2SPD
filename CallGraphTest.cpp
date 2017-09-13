#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

#include <iostream>

using namespace llvm;

class CallGraphTest : public CallGraphSCCPass {
public:
  static char ID;

  CallGraphTest() : CallGraphSCCPass(ID) {
  }

  virtual bool runOnSCC(CallGraphSCC &SCC) override;
  virtual void getAnalysisUsage(AnalysisUsage &AU) const override;
};

bool CallGraphTest::runOnSCC(CallGraphSCC &SCC) {
  for (CallGraphNode *Node : SCC) {
    Function *F = Node->getFunction();
    if (!F) continue;

    std::string OutputFileName = F->getName().str() + ".cg";
    std::error_code EC;
    raw_fd_ostream OS(OutputFileName, EC, sys::fs::F_None);
    if (EC) {
      std::cerr << "cannot create a output file";
      return false;
    }

    OS << "Call Graph for Function: " << F->getName().str() << "\n";
    Node->print(OS);
  }

  return false;
}

void CallGraphTest::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

char CallGraphTest::ID = 0;
static RegisterPass<CallGraphTest> X("cgtest", "test for call graph usage",
                                     false, false);
