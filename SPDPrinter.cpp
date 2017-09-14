#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <iostream>
#include <map>

using namespace llvm;

typedef std::map<Value *, unsigned> ValueNumMapTy;

static unsigned EquCount;
unsigned ValueCount;
ValueNumMapTy ValueNumMap;

static void emitFuncDecl(raw_ostream &OS, Function &F) {
  OS << "Name      " << F.getName().str() << ";\n";

  OS << "Main_In   {Mi::";
  for (const Argument &A : F.args()) {
    OS << A.getName().str() << ", ";
  }
  OS << "__SPD_sop, __SPD_eop};\n";

  OS << "Main_Out  {Mo::";
  Type *RetTy = F.getReturnType();
  switch (RetTy->getTypeID()) {
  case Type::VoidTyID:
    break;
  case Type::IntegerTyID:
  case Type::FloatTyID:
  case Type::DoubleTyID:
    OS << "__SPD_ret, ";
    break;
  default:
    llvm_unreachable("unsupported return type");
  }
  OS << "__SPD_sop, __SPD_eop};\n";
}

// copied from WriteConstantInternal()@IR/AsmPrinter.cpp
static void emitConstantInt(raw_ostream &OS, ConstantInt *CI) {
  if (CI->getType()->isIntegerTy(1)) {
    OS << (CI->getZExtValue() ? "true" : "false");
  }
  else {
    OS << CI->getValue();
  }
}

// copied from WriteConstantInternal()@IR/AsmPrinter.cpp
static void emitConstantFP(raw_ostream &OS, ConstantFP *CFP) {
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

static unsigned getValueNum(Value *V) {
  ValueNumMapTy::iterator Iter = ValueNumMap.find(V);
  if (Iter == ValueNumMap.end()) {
    unsigned RetVal = ValueNumMap[V] = ValueCount;
    ValueCount++;
    return RetVal;
  }
  else {
    return Iter->second;
  }
}

static void emitValue(raw_ostream &OS, Value *V) {
  if (isa<ConstantInt>(V)) {
    emitConstantInt(OS, dyn_cast<ConstantInt>(V));
  }
  else if (isa<ConstantFP>(V)) {
    emitConstantFP(OS, dyn_cast<ConstantFP>(V));
  }
  else {
    if (V->hasName()) {
      OS << V->getName().str();
    }
    else {
      OS << getValueNum(V);
    }
  }
}

static void emitOpcode(raw_ostream &OS, unsigned Opcode) {
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

static void emitEquPrefix(raw_ostream &OS) {
    OS << "EQU       equ" << EquCount << ", ";
    EquCount++;
}

static void emitInstruction(raw_ostream &OS, Instruction &Instr) {
  unsigned NumOperands = Instr.getNumOperands();
  if (Instr.isBinaryOp()) {
    emitEquPrefix(OS);
    emitValue(OS, dyn_cast<Value>(&Instr));
    OS << " = ";
    emitValue(OS, Instr.getOperand(0));
    emitOpcode(OS, Instr.getOpcode());
    emitValue(OS, Instr.getOperand(1));
    OS << ";\n";
  }
  else {
    switch (Instr.getOpcode()) {
    case Instruction::Call:
      emitEquPrefix(OS);
      emitValue(OS, dyn_cast<Value>(&Instr));
      OS << " = ";
      OS << Instr.getOperand(NumOperands - 1)->getName().str();
      OS << "(";
      for (unsigned i = 0; i < (NumOperands - 1); i++) {
        if (i != 0) OS << ", ";
        emitValue(OS, Instr.getOperand(i));
      }
      OS << ")\n";
      break;
    case Instruction::Ret:
      if (NumOperands) {
        emitEquPrefix(OS);
        OS << "__SPD_ret = ";
        emitValue(OS, Instr.getOperand(0));
        OS << ";\n";
      }
      break;
    default:
      llvm_unreachable("unsupported instruction");
    }
  }
}

void emitFunctionSPD(raw_ostream &OS, Function &F) {
  EquCount = 0;
  ValueCount = 0;
  ValueNumMap.clear();

  OS << "// Module " << F.getName().str() << "\n";
  emitFuncDecl(OS, F);

  OS << "\n// equation\n";
  for (BasicBlock &IBB : F) {
    for (BasicBlock::iterator IIB = IBB.begin(), IIE = IBB.end(); IIB != IIE;
         ++IIB) {
      Instruction &Instr = *IIB;
      emitInstruction(OS, Instr);
    }
  }

  OS << "\n// direct connection\n";
  OS << "DRCT      (Mo::__SPD_sop, Mo::__SPD_eop)";
  OS <<        " = (Mi::__SPD_sop, Mi::__SPD_eop);\n";
}
