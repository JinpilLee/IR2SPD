#include "llvm/Pass.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <iostream>

using namespace llvm;

class IR2SPD : public FunctionPass {
public:
  static char ID;

  IR2SPD() : FunctionPass(ID), NodeCount(0) {
  }

  virtual bool runOnFunction(Function &F);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

private:
  unsigned NodeCount;

  void translateFuncDecl(raw_ostream &OS, Function &F) const;

  void translateConstantInt(raw_ostream &OS, ConstantInt *CI) const;
  void translateConstantFP(raw_ostream &OS, ConstantFP *CFP) const;
  void translateValue(raw_ostream &OS, Value *V) const;

  void translateOpcode(raw_ostream &OS, unsigned Opcode) const;
  void emitNodeExpr(raw_ostream &OS);
  void translateInstruction(raw_ostream &OS, Instruction &Instr);
};

bool IR2SPD::runOnFunction(Function &F) {
  // FIXME temporary impl
  NodeCount = 0;

  std::string OutputFileName = F.getName().str() + ".spd";
  std::error_code EC;
  raw_fd_ostream OS(OutputFileName, EC, sys::fs::F_None);
  if (EC) {
    std::cerr << "cannot create a output file";
    return false;
  }

  OS << "// Module: " << F.getName().str() << "\n";
  translateFuncDecl(OS, F);

  OS << "\n// equation\n";
  for (BasicBlock &IBB : F) {
    for (BasicBlock::iterator IIB = IBB.begin(), IIE = IBB.end(); IIB != IIE;
         ++IIB) {
      Instruction &Instr = *IIB;
      translateInstruction(OS, Instr);
    }
  }

  OS << "\n// direct connection\n";
  OS << "DRCT      (Mo::sop, Mo::eop) = (Mi::sop, Mi::eop);\n";

  return false;
}

void IR2SPD::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

void IR2SPD::translateFuncDecl(raw_ostream &OS, Function &F) const {
  OS << "Name      " << F.getName().str() << ";\n";

  OS << "Main_In   {Mi::";
  for (const Argument &A : F.args()) {
    OS << "i_" << A.getName().str() << ", ";
  }
  OS << "sop, eop};\n";

  OS << "Main_Out  {Mo::";
  Type *RetTy = F.getReturnType();
  switch (RetTy->getTypeID()) {
  case Type::VoidTyID:
    break;
  case Type::IntegerTyID:
  case Type::FloatTyID:
  case Type::DoubleTyID:
    OS << "o_ret, ";
    break;
  default:
    llvm_unreachable("unsupported return type");
  }
  OS << "sop, eop};\n";
}

// copied from WriteConstantInternal()@IR/AsmPrinter.cpp
void IR2SPD::translateConstantInt(raw_ostream &OS, ConstantInt *CI) const {
  if (CI->getType()->isIntegerTy(1)) {
    OS << (CI->getZExtValue() ? "true" : "false");
  }
  else {
    OS << CI->getValue();
  }
}

// copied from WriteConstantInternal()@IR/AsmPrinter.cpp
void IR2SPD::translateConstantFP(raw_ostream &OS, ConstantFP *CFP) const {
  if (&CFP->getValueAPF().getSemantics() == &APFloat::IEEEsingle() ||
      &CFP->getValueAPF().getSemantics() == &APFloat::IEEEdouble()) {
    bool ignored;
    bool isDouble = &CFP->getValueAPF().getSemantics()==&APFloat::IEEEdouble();
    bool isInf = CFP->getValueAPF().isInfinity();
    bool isNaN = CFP->getValueAPF().isNaN();
    if (!isInf && !isNaN) {
      double Val = isDouble ? CFP->getValueAPF().convertToDouble() :
                              CFP->getValueAPF().convertToFloat();
      SmallString<128> StrVal;
      raw_svector_ostream(StrVal) << Val;

      if ((StrVal[0] >= '0' && StrVal[0] <= '9') ||
          ((StrVal[0] == '-' || StrVal[0] == '+') &&
           (StrVal[1] >= '0' && StrVal[1] <= '9'))) {
        if (APFloat(APFloat::IEEEdouble(), StrVal).convertToDouble() == Val) {
          OS << StrVal;
          return;
        }
      }
    }
    static_assert(sizeof(double) == sizeof(uint64_t),
                  "assuming that double is 64 bits!");
    APFloat apf = CFP->getValueAPF();
    if (!isDouble)
      apf.convert(APFloat::IEEEdouble(), APFloat::rmNearestTiesToEven,
                        &ignored);
    OS << format_hex(apf.bitcastToAPInt().getZExtValue(), 0, /*Upper=*/true);
    return;
  }

  OS << "0x";
  APInt API = CFP->getValueAPF().bitcastToAPInt();
  if (&CFP->getValueAPF().getSemantics() == &APFloat::x87DoubleExtended()) {
    OS << 'K';
    OS << format_hex_no_prefix(API.getHiBits(16).getZExtValue(), 4,
                               /*Upper=*/true);
    OS << format_hex_no_prefix(API.getLoBits(64).getZExtValue(), 16,
                               /*Upper=*/true);
  }
  else if (&CFP->getValueAPF().getSemantics() == &APFloat::IEEEquad()) {
    OS << 'L';
    OS << format_hex_no_prefix(API.getLoBits(64).getZExtValue(), 16,
                               /*Upper=*/true);
    OS << format_hex_no_prefix(API.getHiBits(64).getZExtValue(), 16,
                               /*Upper=*/true);
  }
  else if (&CFP->getValueAPF().getSemantics() == &APFloat::PPCDoubleDouble()) {
    OS << 'M';
    OS << format_hex_no_prefix(API.getLoBits(64).getZExtValue(), 16,
                               /*Upper=*/true);
    OS << format_hex_no_prefix(API.getHiBits(64).getZExtValue(), 16,
                               /*Upper=*/true);
  }
  else if (&CFP->getValueAPF().getSemantics() == &APFloat::IEEEhalf()) {
    OS << 'H';
    OS << format_hex_no_prefix(API.getZExtValue(), 4,
                               /*Upper=*/true);
  }
  else {
    llvm_unreachable("unsupported floating point type");
  }
}

void IR2SPD::translateValue(raw_ostream &OS, Value *V) const {
  if (isa<ConstantInt>(V)) {
    translateConstantInt(OS, dyn_cast<ConstantInt>(V));
  }
  else if (isa<ConstantFP>(V)) {
    translateConstantFP(OS, dyn_cast<ConstantFP>(V));
  }
  else {
    OS << "t_";
    // FIXME how to remove prefix (%, @)?
    // store output to temporary buffer -> remove
    V->printAsOperand(OS, false);
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
    llvm_unreachable("unsupported opcode");
  }
  OS << " ";
}

void IR2SPD::emitNodeExpr(raw_ostream &OS) {
    OS << "EQU       equ" << NodeCount << ", ";
    NodeCount++;
}

void IR2SPD::translateInstruction(raw_ostream &OS, Instruction &Instr) {
  if (Instr.isBinaryOp()) {
    emitNodeExpr(OS);
    translateValue(OS, dyn_cast<Value>(&Instr));
    OS << " = ";
    translateValue(OS, Instr.getOperand(0));
    translateOpcode(OS, Instr.getOpcode());
    translateValue(OS, Instr.getOperand(1));
    OS << ";\n";
  }
  else {
    switch (Instr.getOpcode()) {
    case Instruction::Ret:
      if (Instr.getNumOperands()) {
        emitNodeExpr(OS);
        OS << "o_ret = ";
        translateValue(OS, Instr.getOperand(0));
        OS << ";\n";
      }
      break;
    default:
      llvm_unreachable("unsupported instruction");
    }
  }
}

char IR2SPD::ID = 0;
static RegisterPass<IR2SPD> X("generate-spd", "generate SPD from LLVM IR",
                              false, false);
