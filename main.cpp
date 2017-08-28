#include "llvm/Pass.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopIterator.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <vector>
#include <iostream>

using namespace llvm;

typedef std::vector<const APInt *> IntParamTy;
typedef std::vector<const APFloat *> FPParamTy;

class IR2SPD : public FunctionPass {
public:
  static char ID;

  IR2SPD() : FunctionPass(ID), NodeCount(0) {
  }

  virtual bool runOnFunction(Function &F);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

private:
  unsigned NodeCount;

  int findIntParam(IntParamTy &IntParams, const APInt &IntValue) const {
    int Idx = 0;
    for (auto P : IntParams) {
      if (*P == IntValue) {
        return Idx;
      }

      Idx++;
    }

    return -1;
  }

  int findFPParam(FPParamTy &FPParams, const APFloat &FPValue) const {
    int Idx = 0;
    for (auto P : FPParams) {
      if (FPValue.bitwiseIsEqual(*P)) {
        return Idx;
      }
    }

    return -1;
  }

  void collectConstantParams(IntParamTy &IntParams, FPParamTy &FPParams,
                             Function &F) const;
  void emitConstantParams(raw_ostream &OS,
                          IntParamTy &IntParams, FPParamTy &FPParams) const;
  void translateValue(raw_ostream &OS, Value *V,
                      IntParamTy &IntParams, FPParamTy &FPParams) const;
  void translateOpcode(raw_ostream &OS, unsigned Opcode) const;
  void translateInstruction(raw_ostream &OS, Instruction &Instr,
                            IntParamTy &IntParams, FPParamTy &FPParams);
};

bool IR2SPD::runOnFunction(Function &F) {
  NodeCount = 0;

  std::string OutputFileName = F.getName().str() + ".spd";
  std::error_code EC;
  raw_fd_ostream OS(OutputFileName, EC, sys::fs::F_None);
  if (EC) {
    std::cerr << "error";
    return false;
  }

  OS << "##### Module: " << F.getName().str() << "\n";

  // print name
  OS << "Name\t\t\t" << F.getName().str() << ";\n";

  // print input ports
  FunctionType *FT = F.getFunctionType();
  unsigned NumParams = FT->getNumParams();
  if (NumParams != 0) {
    OS << "Main_In\t\t\t{main_i::";
    unsigned Idx = 0;
    for (const Argument &A : F.args()) {
      if (Idx != 0) OS << ",";
      Idx++;
      OS << "i_" << A.getName().str();
    }
    OS << "};\n";
  }

  // print output port
  OS << "Main_Out\t\t{main_o::o_0};\n";

  // collect param
  IntParamTy IntParams;
  FPParamTy FPParams;
  collectConstantParams(IntParams, FPParams, F);

  // print param
  OS << "##### Parameters\n";
  emitConstantParams(OS, IntParams, FPParams);

  // print statement
  OS << "##### Statements\n";
  for (BasicBlock &IBB : F) {
    for (BasicBlock::iterator IIB = IBB.begin(), IIE = IBB.end(); IIB != IIE;
         ++IIB) {
      Instruction &Instr = *IIB;
      translateInstruction(OS, Instr, IntParams, FPParams);
    }
  }

  return false;
}

void IR2SPD::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

void IR2SPD::collectConstantParams(IntParamTy &IntParams, FPParamTy &FPParams,
                                   Function &F) const {
  for (BasicBlock &IBB : F) {
    for (BasicBlock::iterator IIB = IBB.begin(), IIE = IBB.end(); IIB != IIE;
         ++IIB) {
      Instruction &Instr = *IIB;
      for (unsigned i = 0, e = Instr.getNumOperands(); i != e; ++i) {
        Value *V = Instr.getOperand(i);
        if (isa<ConstantInt>(V)) {
          const APInt &IntValue = dyn_cast<ConstantInt>(V)->getValue();
          if (findIntParam(IntParams, IntValue) == -1) {
            IntParams.push_back(&IntValue);
          }
        }
        else if (isa<ConstantFP>(V)) {
          const APFloat &FPValue = dyn_cast<ConstantFP>(V)->getValueAPF();
          if (findFPParam(FPParams, FPValue) == -1) {
            FPParams.push_back(&FPValue);
          }
        }
      }
    }
  }
}

void IR2SPD::emitConstantParams(raw_ostream &OS,
                                IntParamTy &IntParams,
                                FPParamTy &FPParams) const {
  int count = 0;
  for (auto P : IntParams) {
    OS << "Param\t\t\tci_" << count << " = ";
    // FIXME signed?
    P->print(OS, true);
    OS << ";\n";
    count++;
  }

  count = 0;
  for (auto P : FPParams) {
    OS << "Param\t\t\tcf_" << count << " = ";
    P->print(OS);
// FIXME APFloat::print() adds newline
//    OS << ";\n";
    count++;
  }
}

void IR2SPD::translateValue(raw_ostream &OS, Value *V,
                            IntParamTy &IntParams, FPParamTy &FPParams) const {
  if (V->hasName()) {
    OS << "t_" << V->getName().str();
  }
  else if (isa<ConstantInt>(V)) {
    OS << "ci_" << findIntParam(IntParams,
                                dyn_cast<ConstantInt>(V)->getValue());
  }
  else if (isa<ConstantFP>(V)) {
    OS << "cf_" << findFPParam(FPParams,
                               dyn_cast<ConstantFP>(V)->getValueAPF());
  }
  else {
    // FIXME check reg
  }
}

void IR2SPD::translateOpcode(raw_ostream &OS, unsigned Opcode) const {
  OS << " ";
  switch (Opcode) {
  case Instruction::Add:
  case Instruction::FAdd:
    OS << "+"; break;
  case Instruction::Sub:
  case Instruction::FSub:
    OS << "-"; break;
  case Instruction::Mul:
  case Instruction::FMul:
    OS << "*"; break;
  case Instruction::UDiv:
  case Instruction::SDiv:
  case Instruction::FDiv:
    OS << "/"; break;
  default:
    llvm_unreachable("wrong opcode");
  }
  OS << " ";
}

void IR2SPD::translateInstruction(raw_ostream &OS, Instruction &Instr,
                                  IntParamTy &IntParams, FPParamTy &FPParams) {
  if (Instr.isBinaryOp()) {
    OS << "EQU Node" << NodeCount << ",\t\t";
    NodeCount++;
    translateValue(OS, dyn_cast<Value>(&Instr), IntParams, FPParams);
    OS << " = ";
    translateValue(OS, Instr.getOperand(0), IntParams, FPParams);
    translateOpcode(OS, Instr.getOpcode());
    translateValue(OS, Instr.getOperand(1), IntParams, FPParams);
    OS << ";\n";
  }
}

char IR2SPD::ID = 0;
static RegisterPass<IR2SPD> X("generate-spd", "generate SPD from LLVM IR",
                              false, false);
