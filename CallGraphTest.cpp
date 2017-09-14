#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CallGraphSCCPass.h"

#include <iostream>

using namespace llvm;

class CallGraphTest : public CallGraphSCCPass {
public:
  static char ID;

  CallGraphTest() : CallGraphSCCPass(ID) {
  }

  virtual bool runOnSCC(CallGraphSCC &SCC) override;
  virtual void getAnalysisUsage(AnalysisUsage &AU) const override;

private:
  void showCalledFunctionRec(CallGraphNode *N);
};

bool CallGraphTest::runOnSCC(CallGraphSCC &SCC) {
  std::cerr << "NEW SCC\n";
  for (CallGraphNode *Node : SCC) {
    Function *F = Node->getFunction();
    if (!F) continue;

    std::string FunctionName = F->getName().str();
    std::cerr << "Call Graph Node for Function: " << FunctionName << "\n";
    if (FunctionName.compare("fpga_main") == 0) {
      showCalledFunctionRec(Node);
    }
  }

  return false;
}

void CallGraphTest::showCalledFunctionRec(CallGraphNode *N) {
  std::string FuncName = N->getFunction()->getName().str();
  for (const auto &I : *N) {
    CallGraphNode *CalledFuncNode = I.second;
    Function *CalledFunc = CalledFuncNode->getFunction();
    if (CalledFunc) {
      std::cerr << FuncName
        << " calls function: " << CalledFunc->getName().str() << "\n";
      showCalledFunctionRec(CalledFuncNode);
    }
    else {
      std::cerr << FuncName << " calls external node\n";
    }
  }
}

void CallGraphTest::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

char CallGraphTest::ID = 0;
static RegisterPass<CallGraphTest> X("cgtest", "test for call graph usage",
                                     false, false);
