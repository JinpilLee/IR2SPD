#include "llvm/IR/Function.h"
#include <string>

using namespace llvm;

// XXX std::string or Function *
// which one is better?
typedef std::map<std::string, unsigned> SPDModuleMapTy;

void emitFunctionSPD(Function &F, SPDModuleMapTy &SPDModuleMap);
