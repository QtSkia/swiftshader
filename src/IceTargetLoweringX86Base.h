//===- subzero/src/IceTargetLoweringX86Base.h - x86 lowering ----*- C++ -*-===//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Declares the TargetLoweringX86 template class, which implements the
/// TargetLowering base interface for the x86 architecture.
///
//===----------------------------------------------------------------------===//

#ifndef SUBZERO_SRC_ICETARGETLOWERINGX86BASE_H
#define SUBZERO_SRC_ICETARGETLOWERINGX86BASE_H

#include "IceDefs.h"
#include "IceInst.h"
#include "IceSwitchLowering.h"
#include "IceTargetLowering.h"
#include "IceTargetLoweringX86RegClass.h"
#include "IceUtils.h"

#include <array>
#include <type_traits>
#include <utility>

#ifndef X86NAMESPACE
#error "You must define the X86 Target namespace."
#endif

namespace Ice {
namespace X86NAMESPACE {

using namespace ::Ice::X86;

template <typename Traits> class BoolFolding;

/// TargetX86Base is a template for all X86 Targets, and it relies on the CRT
/// pattern for generating code, delegating to actual backends target-specific
/// lowerings (e.g., call, ret, and intrinsics.) Backends are expected to
/// implement the following methods (which should be accessible from
/// TargetX86Base):
///
/// Operand *createNaClReadTPSrcOperand()
///
/// Note: Ideally, we should be able to
///
///  static_assert(std::is_base_of<TargetX86Base<TraitsType>,
///  Machine>::value);
///
/// but that does not work: the compiler does not know that Machine inherits
/// from TargetX86Base at this point in translation.
template <typename TraitsType> class TargetX86Base : public TargetLowering {
  TargetX86Base() = delete;
  TargetX86Base(const TargetX86Base &) = delete;
  TargetX86Base &operator=(const TargetX86Base &) = delete;

public:
  using Traits = TraitsType;
  using ConcreteTarget = typename Traits::ConcreteTarget;
  using InstructionSetEnum = typename Traits::InstructionSet;

  using BrCond = typename Traits::Cond::BrCond;
  using CmppsCond = typename Traits::Cond::CmppsCond;

  using X86Address = typename Traits::Address;
  using X86Operand = typename Traits::X86Operand;
  using X86OperandMem = typename Traits::X86OperandMem;
  using SegmentRegisters = typename Traits::X86OperandMem::SegmentRegisters;
  using SpillVariable = typename Traits::SpillVariable;

  using InstX86Br = typename Traits::Insts::Br;
  using InstX86FakeRMW = typename Traits::Insts::FakeRMW;
  using InstX86Label = typename Traits::Insts::Label;

  ~TargetX86Base() override = default;

  static void staticInit(const ClFlags &Flags);
  static TargetX86Base *create(Cfg *Func) { return new TargetX86Base(Func); }

  static FixupKind getPcRelFixup() { return PcRelFixup; }
  static FixupKind getAbsFixup() { return AbsFixup; }

  bool needSandboxing() const { return NeedSandboxing; }

  void translateOm1() override;
  void translateO2() override;
  void doLoadOpt();
  bool doBranchOpt(Inst *I, const CfgNode *NextNode) override;

  SizeT getNumRegisters() const override {
    return Traits::RegisterSet::Reg_NUM;
  }
  Variable *getPhysicalRegister(SizeT RegNum, Type Ty = IceType_void) override;
  IceString getRegName(SizeT RegNum, Type Ty) const override;
  llvm::SmallBitVector getRegisterSet(RegSetMask Include,
                                      RegSetMask Exclude) const override;
  const llvm::SmallBitVector &
  getRegistersForVariable(const Variable *Var) const override {
    RegClass RC = Var->getRegClass();
    assert(static_cast<RegClassX86>(RC) < RCX86_NUM);
    return TypeToRegisterSet[RC];
  }

  const llvm::SmallBitVector &getAliasesForRegister(SizeT Reg) const override {
    assert(Reg < Traits::RegisterSet::Reg_NUM);
    return RegisterAliases[Reg];
  }

  bool hasFramePointer() const override { return IsEbpBasedFrame; }
  void setHasFramePointer() override { IsEbpBasedFrame = true; }
  SizeT getStackReg() const override { return Traits::StackPtr; }
  SizeT getFrameReg() const override { return Traits::FramePtr; }
  SizeT getFrameOrStackReg() const override {
    return IsEbpBasedFrame ? getFrameReg() : getStackReg();
  }
  size_t typeWidthInBytesOnStack(Type Ty) const override {
    // Round up to the next multiple of WordType bytes.
    const uint32_t WordSizeInBytes = typeWidthInBytes(Traits::WordType);
    return Utils::applyAlignment(typeWidthInBytes(Ty), WordSizeInBytes);
  }
  uint32_t getStackAlignment() const override {
    return Traits::X86_STACK_ALIGNMENT_BYTES;
  }
  void reserveFixedAllocaArea(size_t Size, size_t Align) override {
    FixedAllocaSizeBytes = Size;
    assert(llvm::isPowerOf2_32(Align));
    FixedAllocaAlignBytes = Align;
    PrologEmitsFixedAllocas = true;
  }
  /// Returns the (negative) offset from ebp/rbp where the fixed Allocas start.
  int32_t getFrameFixedAllocaOffset() const override {
    return FixedAllocaSizeBytes - (SpillAreaSizeBytes - maxOutArgsSizeBytes());
  }
  virtual uint32_t maxOutArgsSizeBytes() const override {
    return MaxOutArgsSizeBytes;
  }
  virtual void updateMaxOutArgsSizeBytes(uint32_t Size) {
    MaxOutArgsSizeBytes = std::max(MaxOutArgsSizeBytes, Size);
  }

  bool shouldSplitToVariable64On32(Type Ty) const override {
    return Traits::Is64Bit ? false : Ty == IceType_i64;
  }

  SizeT getMinJumpTableSize() const override { return 4; }

  void emitVariable(const Variable *Var) const override;

  void emit(const ConstantInteger32 *C) const final;
  void emit(const ConstantInteger64 *C) const final;
  void emit(const ConstantFloat *C) const final;
  void emit(const ConstantDouble *C) const final;
  void emit(const ConstantUndef *C) const final;
  void emit(const ConstantRelocatable *C) const final;

  void initNodeForLowering(CfgNode *Node) override;

  template <typename T = Traits>
  typename std::enable_if<!T::Is64Bit, Operand>::type *
  loOperand(Operand *Operand);
  template <typename T = Traits>
  typename std::enable_if<T::Is64Bit, Operand>::type *loOperand(Operand *) {
    llvm::report_fatal_error(
        "Hey, yo! This is x86-64. Watcha doin'? (loOperand)");
  }

  template <typename T = Traits>
  typename std::enable_if<!T::Is64Bit, Operand>::type *
  hiOperand(Operand *Operand);
  template <typename T = Traits>
  typename std::enable_if<T::Is64Bit, Operand>::type *hiOperand(Operand *) {
    llvm::report_fatal_error(
        "Hey, yo! This is x86-64. Watcha doin'? (hiOperand)");
  }

  void finishArgumentLowering(Variable *Arg, Variable *FramePtr,
                              size_t BasicFrameOffset, size_t StackAdjBytes,
                              size_t &InArgsSizeBytes);
  X86Address stackVarToAsmOperand(const Variable *Var) const;

  InstructionSetEnum getInstructionSet() const { return InstructionSet; }
  Operand *legalizeUndef(Operand *From, int32_t RegNum = Variable::NoRegister);

protected:
  const bool NeedSandboxing;

  explicit TargetX86Base(Cfg *Func);

  void postLower() override;

  void initSandbox() {
    dispatchToConcrete(&Traits::ConcreteTarget::initSandbox);
  }

  void lowerAlloca(const InstAlloca *Inst) override;
  void lowerArithmetic(const InstArithmetic *Inst) override;
  void lowerAssign(const InstAssign *Inst) override;
  void lowerBr(const InstBr *Inst) override;
  void lowerCast(const InstCast *Inst) override;
  void lowerExtractElement(const InstExtractElement *Inst) override;
  void lowerFcmp(const InstFcmp *Inst) override;
  void lowerIcmp(const InstIcmp *Inst) override;

  void lowerIntrinsicCall(const InstIntrinsicCall *Inst) override;
  void lowerInsertElement(const InstInsertElement *Inst) override;
  void lowerLoad(const InstLoad *Inst) override;
  void lowerPhi(const InstPhi *Inst) override;
  void lowerSelect(const InstSelect *Inst) override;
  void lowerStore(const InstStore *Inst) override;
  void lowerSwitch(const InstSwitch *Inst) override;
  void lowerUnreachable(const InstUnreachable *Inst) override;
  void lowerOther(const Inst *Instr) override;
  void lowerRMW(const InstX86FakeRMW *RMW);
  void prelowerPhis() override;
  uint32_t getCallStackArgumentsSizeBytes(const std::vector<Type> &ArgTypes,
                                          Type ReturnType);
  uint32_t getCallStackArgumentsSizeBytes(const InstCall *Instr) override;
  void genTargetHelperCallFor(Inst *Instr) override;
  void doAddressOptLoad() override;
  void doAddressOptStore() override;
  void doMockBoundsCheck(Operand *Opnd) override;
  void randomlyInsertNop(float Probability,
                         RandomNumberGenerator &RNG) override;

  /// Naive lowering of cmpxchg.
  void lowerAtomicCmpxchg(Variable *DestPrev, Operand *Ptr, Operand *Expected,
                          Operand *Desired);
  /// Attempt a more optimized lowering of cmpxchg. Returns true if optimized.
  bool tryOptimizedCmpxchgCmpBr(Variable *DestPrev, Operand *Ptr,
                                Operand *Expected, Operand *Desired);
  void lowerAtomicRMW(Variable *Dest, uint32_t Operation, Operand *Ptr,
                      Operand *Val);
  void lowerCountZeros(bool Cttz, Type Ty, Variable *Dest, Operand *FirstVal,
                       Operand *SecondVal);
  /// Load from memory for a given type.
  void typedLoad(Type Ty, Variable *Dest, Variable *Base, Constant *Offset);
  /// Store to memory for a given type.
  void typedStore(Type Ty, Variable *Value, Variable *Base, Constant *Offset);
  /// Copy memory of given type from Src to Dest using OffsetAmt on both.
  void copyMemory(Type Ty, Variable *Dest, Variable *Src, int32_t OffsetAmt);
  /// Replace some calls to memcpy with inline instructions.
  void lowerMemcpy(Operand *Dest, Operand *Src, Operand *Count);
  /// Replace some calls to memmove with inline instructions.
  void lowerMemmove(Operand *Dest, Operand *Src, Operand *Count);
  /// Replace some calls to memset with inline instructions.
  void lowerMemset(Operand *Dest, Operand *Val, Operand *Count);

  /// Lower an indirect jump adding sandboxing when needed.
  void lowerIndirectJump(Variable *JumpTarget) {
    // Without std::move below, the compiler deduces that the argument to
    // lowerIndirectJmp is a Variable *&, not a Variable *.
    dispatchToConcrete(&Traits::ConcreteTarget::lowerIndirectJump,
                       std::move(JumpTarget));
  }

  /// Check the comparison is in [Min,Max]. The flags register will be modified
  /// with:
  ///   - below equal, if in range
  ///   - above, set if not in range
  /// The index into the range is returned.
  Operand *lowerCmpRange(Operand *Comparison, uint64_t Min, uint64_t Max);
  /// Lowering of a cluster of switch cases. If the case is not matched control
  /// will pass to the default label provided. If the default label is nullptr
  /// then control will fall through to the next instruction. DoneCmp should be
  /// true if the flags contain the result of a comparison with the Comparison.
  void lowerCaseCluster(const CaseCluster &Case, Operand *Src0, bool DoneCmp,
                        CfgNode *DefaultLabel = nullptr);

  using LowerBinOp = void (TargetX86Base::*)(Variable *, Operand *);
  void expandAtomicRMWAsCmpxchg(LowerBinOp op_lo, LowerBinOp op_hi,
                                Variable *Dest, Operand *Ptr, Operand *Val);

  void eliminateNextVectorSextInstruction(Variable *SignExtendedResult);

  void scalarizeArithmetic(InstArithmetic::OpKind K, Variable *Dest,
                           Operand *Src0, Operand *Src1);

  /// Emit a fake use of esp to make sure esp stays alive for the entire
  /// function. Otherwise some esp adjustments get dead-code eliminated.
  void keepEspLiveAtExit() {
    Variable *esp =
        Func->getTarget()->getPhysicalRegister(getStackReg(), Traits::WordType);
    Context.insert<InstFakeUse>(esp);
  }

  /// Operand legalization helpers. To deal with address mode constraints, the
  /// helpers will create a new Operand and emit instructions that guarantee
  /// that the Operand kind is one of those indicated by the LegalMask (a
  /// bitmask of allowed kinds). If the input Operand is known to already meet
  /// the constraints, it may be simply returned as the result, without creating
  /// any new instructions or operands.
  enum OperandLegalization {
    Legal_None = 0,
    Legal_Reg = 1 << 0, // physical register, not stack location
    Legal_Imm = 1 << 1,
    Legal_Mem = 1 << 2, // includes [eax+4*ecx] as well as [esp+12]
    Legal_Rematerializable = 1 << 3,
    Legal_AddrAbs = 1 << 4, // ConstantRelocatable doesn't have to add GotVar
    Legal_Default = ~(Legal_Rematerializable | Legal_AddrAbs)
    // TODO(stichnot): Figure out whether this default works for x86-64.
  };
  using LegalMask = uint32_t;
  Operand *legalize(Operand *From, LegalMask Allowed = Legal_Default,
                    int32_t RegNum = Variable::NoRegister);
  Variable *legalizeToReg(Operand *From, int32_t RegNum = Variable::NoRegister);
  /// Legalize the first source operand for use in the cmp instruction.
  Operand *legalizeSrc0ForCmp(Operand *Src0, Operand *Src1);
  /// Turn a pointer operand into a memory operand that can be used by a real
  /// load/store operation. Legalizes the operand as well. This is a nop if the
  /// operand is already a legal memory operand.
  X86OperandMem *formMemoryOperand(Operand *Ptr, Type Ty,
                                   bool DoLegalize = true);

  Variable *makeReg(Type Ty, int32_t RegNum = Variable::NoRegister);
  static Type stackSlotType();

  static constexpr uint32_t NoSizeLimit = 0;
  static const Type TypeForSize[];
  /// Returns the largest type which is equal to or larger than Size bytes. The
  /// type is suitable for copying memory i.e. a load and store will be a single
  /// instruction (for example x86 will get f64 not i64).
  static Type largestTypeInSize(uint32_t Size, uint32_t MaxSize = NoSizeLimit);
  /// Returns the smallest type which is equal to or larger than Size bytes. If
  /// one doesn't exist then the largest type smaller than Size bytes is
  /// returned. The type is suitable for memory copies as described at
  /// largestTypeInSize.
  static Type firstTypeThatFitsSize(uint32_t Size,
                                    uint32_t MaxSize = NoSizeLimit);

  Variable *copyToReg8(Operand *Src, int32_t RegNum = Variable::NoRegister);
  Variable *copyToReg(Operand *Src, int32_t RegNum = Variable::NoRegister);

  /// Returns a register containing all zeros, without affecting the FLAGS
  /// register, using the best instruction for the type.
  Variable *makeZeroedRegister(Type Ty, int32_t RegNum = Variable::NoRegister);

  /// \name Returns a vector in a register with the given constant entries.
  /// @{
  Variable *makeVectorOfZeros(Type Ty, int32_t RegNum = Variable::NoRegister);
  Variable *makeVectorOfOnes(Type Ty, int32_t RegNum = Variable::NoRegister);
  Variable *makeVectorOfMinusOnes(Type Ty,
                                  int32_t RegNum = Variable::NoRegister);
  Variable *makeVectorOfHighOrderBits(Type Ty,
                                      int32_t RegNum = Variable::NoRegister);
  Variable *makeVectorOfFabsMask(Type Ty,
                                 int32_t RegNum = Variable::NoRegister);
  /// @}

  /// Return a memory operand corresponding to a stack allocated Variable.
  X86OperandMem *getMemoryOperandForStackSlot(Type Ty, Variable *Slot,
                                              uint32_t Offset = 0);

  void
  makeRandomRegisterPermutation(llvm::SmallVectorImpl<int32_t> &Permutation,
                                const llvm::SmallBitVector &ExcludeRegisters,
                                uint64_t Salt) const override;

  /// AutoMemorySandboxer emits a bundle-lock/bundle-unlock pair if the
  /// instruction's operand is a memory reference. This is only needed for
  /// x86-64 NaCl sandbox.
  template <InstBundleLock::Option BundleLockOpt = InstBundleLock::Opt_None>
  class AutoMemorySandboxer {
    AutoMemorySandboxer() = delete;
    AutoMemorySandboxer(const AutoMemorySandboxer &) = delete;
    AutoMemorySandboxer &operator=(const AutoMemorySandboxer &) = delete;

  private:
    typename Traits::TargetLowering *Target;

    template <typename T, typename... Tail>
    X86OperandMem **findMemoryReference(T **First, Tail... Others) {
      if (llvm::isa<X86OperandMem>(*First)) {
        return reinterpret_cast<X86OperandMem **>(First);
      }
      return findMemoryReference(Others...);
    }

    X86OperandMem **findMemoryReference() { return nullptr; }

  public:
    X86OperandMem **const MemOperand;

    template <typename... T>
    AutoMemorySandboxer(typename Traits::TargetLowering *Target, T... Args)
        : Target(Target),
          MemOperand(
              (!Traits::Is64Bit || !Target->Ctx->getFlags().getUseSandboxing())
                  ? nullptr
                  : findMemoryReference(Args...)) {
      if (MemOperand != nullptr) {
        Target->_bundle_lock(BundleLockOpt);
        *MemOperand = Target->_sandbox_mem_reference(*MemOperand);
      }
    }

    ~AutoMemorySandboxer() {
      if (MemOperand != nullptr) {
        Target->_bundle_unlock();
      }
    }
  };

  /// The following are helpers that insert lowered x86 instructions with
  /// minimal syntactic overhead, so that the lowering code can look as close to
  /// assembly as practical.
  void _adc(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Adc>(Dest, Src0);
  }
  void _adc_rmw(X86OperandMem *DestSrc0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &DestSrc0, &Src1);
    Context.insert<typename Traits::Insts::AdcRMW>(DestSrc0, Src1);
  }
  void _add(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Add>(Dest, Src0);
  }
  void _add_rmw(X86OperandMem *DestSrc0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &DestSrc0, &Src1);
    Context.insert<typename Traits::Insts::AddRMW>(DestSrc0, Src1);
  }
  void _addps(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Addps>(Dest, Src0);
  }
  void _addss(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Addss>(Dest, Src0);
  }
  void _add_sp(Operand *Adjustment) {
    dispatchToConcrete(&Traits::ConcreteTarget::_add_sp, std::move(Adjustment));
  }
  void _and(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::And>(Dest, Src0);
  }
  void _andnps(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Andnps>(Dest, Src0);
  }
  void _andps(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Andps>(Dest, Src0);
  }
  void _and_rmw(X86OperandMem *DestSrc0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &DestSrc0, &Src1);
    Context.insert<typename Traits::Insts::AndRMW>(DestSrc0, Src1);
  }
  void _blendvps(Variable *Dest, Operand *Src0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Blendvps>(Dest, Src0, Src1);
  }
  void _br(BrCond Condition, CfgNode *TargetTrue, CfgNode *TargetFalse) {
    Context.insert<InstX86Br>(TargetTrue, TargetFalse, Condition,
                              InstX86Br::Far);
  }
  void _br(CfgNode *Target) {
    Context.insert<InstX86Br>(Target, InstX86Br::Far);
  }
  void _br(BrCond Condition, CfgNode *Target) {
    Context.insert<InstX86Br>(Target, Condition, InstX86Br::Far);
  }
  void _br(BrCond Condition, InstX86Label *Label,
           typename InstX86Br::Mode Kind = InstX86Br::Near) {
    Context.insert<InstX86Br>(Label, Condition, Kind);
  }
  void _bsf(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Bsf>(Dest, Src0);
  }
  void _bsr(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Bsr>(Dest, Src0);
  }
  void _bswap(Variable *SrcDest) {
    AutoMemorySandboxer<> _(this, &SrcDest);
    Context.insert<typename Traits::Insts::Bswap>(SrcDest);
  }
  void _cbwdq(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Cbwdq>(Dest, Src0);
  }
  void _cmov(Variable *Dest, Operand *Src0, BrCond Condition) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Cmov>(Dest, Src0, Condition);
  }
  void _cmp(Operand *Src0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &Src0, &Src1);
    Context.insert<typename Traits::Insts::Icmp>(Src0, Src1);
  }
  void _cmpps(Variable *Dest, Operand *Src0, CmppsCond Condition) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Cmpps>(Dest, Src0, Condition);
  }
  void _cmpxchg(Operand *DestOrAddr, Variable *Eax, Variable *Desired,
                bool Locked) {
    AutoMemorySandboxer<> _(this, &DestOrAddr);
    Context.insert<typename Traits::Insts::Cmpxchg>(DestOrAddr, Eax, Desired,
                                                    Locked);
    // Mark eax as possibly modified by cmpxchg.
    Context.insert<InstFakeDef>(Eax, llvm::dyn_cast<Variable>(DestOrAddr));
    _set_dest_redefined();
    Context.insert<InstFakeUse>(Eax);
  }
  void _cmpxchg8b(X86OperandMem *Addr, Variable *Edx, Variable *Eax,
                  Variable *Ecx, Variable *Ebx, bool Locked) {
    AutoMemorySandboxer<> _(this, &Addr);
    Context.insert<typename Traits::Insts::Cmpxchg8b>(Addr, Edx, Eax, Ecx, Ebx,
                                                      Locked);
    // Mark edx, and eax as possibly modified by cmpxchg8b.
    Context.insert<InstFakeDef>(Edx);
    _set_dest_redefined();
    Context.insert<InstFakeUse>(Edx);
    Context.insert<InstFakeDef>(Eax);
    _set_dest_redefined();
    Context.insert<InstFakeUse>(Eax);
  }
  void _cvt(Variable *Dest, Operand *Src0,
            typename Traits::Insts::Cvt::CvtVariant Variant) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Cvt>(Dest, Src0, Variant);
  }
  void _div(Variable *Dest, Operand *Src0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0, &Src1);
    Context.insert<typename Traits::Insts::Div>(Dest, Src0, Src1);
  }
  void _divps(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Divps>(Dest, Src0);
  }
  void _divss(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Divss>(Dest, Src0);
  }
  template <typename T = Traits>
  typename std::enable_if<T::UsesX87, void>::type _fld(Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Src0);
    Context.insert<typename Traits::Insts::template Fld<>>(Src0);
  }
  // TODO(jpp): when implementing the X8664 calling convention, make sure x8664
  // does not invoke this method, and remove it.
  template <typename T = Traits>
  typename std::enable_if<!T::UsesX87, void>::type _fld(Operand *) {
    llvm::report_fatal_error("fld is not available in x86-64");
  }
  template <typename T = Traits>
  typename std::enable_if<T::UsesX87, void>::type _fstp(Variable *Dest) {
    AutoMemorySandboxer<> _(this, &Dest);
    Context.insert<typename Traits::Insts::template Fstp<>>(Dest);
  }
  // TODO(jpp): when implementing the X8664 calling convention, make sure x8664
  // does not invoke this method, and remove it.
  template <typename T = Traits>
  typename std::enable_if<!T::UsesX87, void>::type _fstp(Variable *) {
    llvm::report_fatal_error("fstp is not available in x86-64");
  }
  void _idiv(Variable *Dest, Operand *Src0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0, &Src1);
    Context.insert<typename Traits::Insts::Idiv>(Dest, Src0, Src1);
  }
  void _imul(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Imul>(Dest, Src0);
  }
  void _imul_imm(Variable *Dest, Operand *Src0, Constant *Imm) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::ImulImm>(Dest, Src0, Imm);
  }
  void _insertps(Variable *Dest, Operand *Src0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0, &Src1);
    Context.insert<typename Traits::Insts::Insertps>(Dest, Src0, Src1);
  }
  void _jmp(Operand *Target) {
    AutoMemorySandboxer<> _(this, &Target);
    Context.insert<typename Traits::Insts::Jmp>(Target);
  }
  void _lea(Variable *Dest, Operand *Src0) {
    Context.insert<typename Traits::Insts::Lea>(Dest, Src0);
  }
  void _mfence() { Context.insert<typename Traits::Insts::Mfence>(); }
  /// Moves can be used to redefine registers, creating "partial kills" for
  /// liveness.  Mark where moves are used in this way.
  void _redefined(Inst *MovInst, bool IsRedefinition = true) {
    if (IsRedefinition)
      MovInst->setDestRedefined();
  }
  /// If Dest=nullptr is passed in, then a new variable is created, marked as
  /// infinite register allocation weight, and returned through the in/out Dest
  /// argument.
  typename Traits::Insts::Mov *_mov(Variable *&Dest, Operand *Src0,
                                    int32_t RegNum = Variable::NoRegister) {
    if (Dest == nullptr)
      Dest = makeReg(Src0->getType(), RegNum);
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    return Context.insert<typename Traits::Insts::Mov>(Dest, Src0);
  }
  void _mov_sp(Operand *NewValue) {
    dispatchToConcrete(&Traits::ConcreteTarget::_mov_sp, std::move(NewValue));
  }
  typename Traits::Insts::Movp *_movp(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    return Context.insert<typename Traits::Insts::Movp>(Dest, Src0);
  }
  void _movd(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Movd>(Dest, Src0);
  }
  void _movq(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Movq>(Dest, Src0);
  }
  void _movss(Variable *Dest, Variable *Src0) {
    Context.insert<typename Traits::Insts::MovssRegs>(Dest, Src0);
  }
  void _movsx(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Movsx>(Dest, Src0);
  }
  typename Traits::Insts::Movzx *_movzx(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    return Context.insert<typename Traits::Insts::Movzx>(Dest, Src0);
  }
  void _maxss(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Maxss>(Dest, Src0);
  }
  void _minss(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Minss>(Dest, Src0);
  }
  void _mul(Variable *Dest, Variable *Src0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0, &Src1);
    Context.insert<typename Traits::Insts::Mul>(Dest, Src0, Src1);
  }
  void _mulps(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Mulps>(Dest, Src0);
  }
  void _mulss(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Mulss>(Dest, Src0);
  }
  void _neg(Variable *SrcDest) {
    AutoMemorySandboxer<> _(this, &SrcDest);
    Context.insert<typename Traits::Insts::Neg>(SrcDest);
  }
  void _nop(SizeT Variant) {
    Context.insert<typename Traits::Insts::Nop>(Variant);
  }
  void _or(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Or>(Dest, Src0);
  }
  void _orps(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Orps>(Dest, Src0);
  }
  void _or_rmw(X86OperandMem *DestSrc0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &DestSrc0, &Src1);
    Context.insert<typename Traits::Insts::OrRMW>(DestSrc0, Src1);
  }
  void _padd(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Padd>(Dest, Src0);
  }
  void _pand(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Pand>(Dest, Src0);
  }
  void _pandn(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Pandn>(Dest, Src0);
  }
  void _pblendvb(Variable *Dest, Operand *Src0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0, &Src1);
    Context.insert<typename Traits::Insts::Pblendvb>(Dest, Src0, Src1);
  }
  void _pcmpeq(Variable *Dest, Operand *Src0,
               Type ArithmeticTypeOverride = IceType_void) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Pcmpeq>(Dest, Src0,
                                                   ArithmeticTypeOverride);
  }
  void _pcmpgt(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Pcmpgt>(Dest, Src0);
  }
  void _pextr(Variable *Dest, Operand *Src0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0, &Src1);
    Context.insert<typename Traits::Insts::Pextr>(Dest, Src0, Src1);
  }
  void _pinsr(Variable *Dest, Operand *Src0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0, &Src1);
    Context.insert<typename Traits::Insts::Pinsr>(Dest, Src0, Src1);
  }
  void _pmull(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Pmull>(Dest, Src0);
  }
  void _pmuludq(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Pmuludq>(Dest, Src0);
  }
  void _pop(Variable *Dest) {
    Context.insert<typename Traits::Insts::Pop>(Dest);
  }
  void _por(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Por>(Dest, Src0);
  }
  void _pshufd(Variable *Dest, Operand *Src0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0, &Src1);
    Context.insert<typename Traits::Insts::Pshufd>(Dest, Src0, Src1);
  }
  void _psll(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Psll>(Dest, Src0);
  }
  void _psra(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Psra>(Dest, Src0);
  }
  void _psrl(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Psrl>(Dest, Src0);
  }
  void _psub(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Psub>(Dest, Src0);
  }
  void _push(Operand *Src0) {
    Context.insert<typename Traits::Insts::Push>(Src0);
  }
  void _pxor(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Pxor>(Dest, Src0);
  }
  void _ret(Variable *Src0 = nullptr) {
    Context.insert<typename Traits::Insts::Ret>(Src0);
  }
  void _rol(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Rol>(Dest, Src0);
  }
  X86OperandMem *_sandbox_mem_reference(X86OperandMem *Mem) {
    return dispatchToConcrete(&Traits::ConcreteTarget::_sandbox_mem_reference,
                              std::move(Mem));
  }
  void _sar(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Sar>(Dest, Src0);
  }
  void _sbb(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Sbb>(Dest, Src0);
  }
  void _sbb_rmw(X86OperandMem *DestSrc0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &DestSrc0, &Src1);
    Context.insert<typename Traits::Insts::SbbRMW>(DestSrc0, Src1);
  }
  void _setcc(Variable *Dest, BrCond Condition) {
    Context.insert<typename Traits::Insts::Setcc>(Dest, Condition);
  }
  void _shl(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Shl>(Dest, Src0);
  }
  void _shld(Variable *Dest, Variable *Src0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0, &Src1);
    Context.insert<typename Traits::Insts::Shld>(Dest, Src0, Src1);
  }
  void _shr(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Shr>(Dest, Src0);
  }
  void _shrd(Variable *Dest, Variable *Src0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0, &Src1);
    Context.insert<typename Traits::Insts::Shrd>(Dest, Src0, Src1);
  }
  void _shufps(Variable *Dest, Operand *Src0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0, &Src1);
    Context.insert<typename Traits::Insts::Shufps>(Dest, Src0, Src1);
  }
  void _sqrtss(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Sqrtss>(Dest, Src0);
  }
  void _store(Operand *Value, X86Operand *Mem) {
    AutoMemorySandboxer<> _(this, &Value, &Mem);
    Context.insert<typename Traits::Insts::Store>(Value, Mem);
  }
  void _storep(Variable *Value, X86OperandMem *Mem) {
    AutoMemorySandboxer<> _(this, &Value, &Mem);
    Context.insert<typename Traits::Insts::StoreP>(Value, Mem);
  }
  void _storeq(Variable *Value, X86OperandMem *Mem) {
    AutoMemorySandboxer<> _(this, &Value, &Mem);
    Context.insert<typename Traits::Insts::StoreQ>(Value, Mem);
  }
  void _sub(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Sub>(Dest, Src0);
  }
  void _sub_rmw(X86OperandMem *DestSrc0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &DestSrc0, &Src1);
    Context.insert<typename Traits::Insts::SubRMW>(DestSrc0, Src1);
  }
  void _sub_sp(Operand *Adjustment) {
    dispatchToConcrete(&Traits::ConcreteTarget::_sub_sp, std::move(Adjustment));
  }
  void _subps(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Subps>(Dest, Src0);
  }
  void _subss(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Subss>(Dest, Src0);
  }
  void _test(Operand *Src0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &Src0, &Src1);
    Context.insert<typename Traits::Insts::Test>(Src0, Src1);
  }
  void _ucomiss(Operand *Src0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &Src0, &Src1);
    Context.insert<typename Traits::Insts::Ucomiss>(Src0, Src1);
  }
  void _ud2() { Context.insert<typename Traits::Insts::UD2>(); }
  void _xadd(Operand *Dest, Variable *Src, bool Locked) {
    AutoMemorySandboxer<> _(this, &Dest, &Src);
    Context.insert<typename Traits::Insts::Xadd>(Dest, Src, Locked);
    // The xadd exchanges Dest and Src (modifying Src). Model that update with
    // a FakeDef followed by a FakeUse.
    Context.insert<InstFakeDef>(Src, llvm::dyn_cast<Variable>(Dest));
    _set_dest_redefined();
    Context.insert<InstFakeUse>(Src);
  }
  void _xchg(Operand *Dest, Variable *Src) {
    AutoMemorySandboxer<> _(this, &Dest, &Src);
    Context.insert<typename Traits::Insts::Xchg>(Dest, Src);
    // The xchg modifies Dest and Src -- model that update with a
    // FakeDef/FakeUse.
    Context.insert<InstFakeDef>(Src, llvm::dyn_cast<Variable>(Dest));
    _set_dest_redefined();
    Context.insert<InstFakeUse>(Src);
  }
  void _xor(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Xor>(Dest, Src0);
  }
  void _xorps(Variable *Dest, Operand *Src0) {
    AutoMemorySandboxer<> _(this, &Dest, &Src0);
    Context.insert<typename Traits::Insts::Xorps>(Dest, Src0);
  }
  void _xor_rmw(X86OperandMem *DestSrc0, Operand *Src1) {
    AutoMemorySandboxer<> _(this, &DestSrc0, &Src1);
    Context.insert<typename Traits::Insts::XorRMW>(DestSrc0, Src1);
  }

  void _iaca_start() {
    if (!BuildDefs::minimal())
      Context.insert<typename Traits::Insts::IacaStart>();
  }
  void _iaca_end() {
    if (!BuildDefs::minimal())
      Context.insert<typename Traits::Insts::IacaEnd>();
  }

  /// This class helps wrap IACA markers around the code generated by the
  /// current scope. It means you don't need to put an end before each return.
  class ScopedIacaMark {
    ScopedIacaMark(const ScopedIacaMark &) = delete;
    ScopedIacaMark &operator=(const ScopedIacaMark &) = delete;

  public:
    ScopedIacaMark(TargetX86Base *Lowering) : Lowering(Lowering) {
      Lowering->_iaca_start();
    }
    ~ScopedIacaMark() { end(); }
    void end() {
      if (!Lowering)
        return;
      Lowering->_iaca_end();
      Lowering = nullptr;
    }

  private:
    TargetX86Base *Lowering;
  };

  bool optimizeScalarMul(Variable *Dest, Operand *Src0, int32_t Src1);
  void findRMW();

  InstructionSetEnum InstructionSet = Traits::InstructionSet::Begin;
  bool IsEbpBasedFrame = false;
  bool NeedsStackAlignment = false;
  size_t SpillAreaSizeBytes = 0;
  size_t FixedAllocaSizeBytes = 0;
  size_t FixedAllocaAlignBytes = 0;
  bool PrologEmitsFixedAllocas = false;
  uint32_t MaxOutArgsSizeBytes = 0;
  static std::array<llvm::SmallBitVector, RCX86_NUM> TypeToRegisterSet;
  static std::array<llvm::SmallBitVector, Traits::RegisterSet::Reg_NUM>
      RegisterAliases;
  static llvm::SmallBitVector ScratchRegs;
  llvm::SmallBitVector RegsUsed;
  std::array<VarList, IceType_NUM> PhysicalRegisters;
  // GotVar is a Variable that holds the GlobalOffsetTable address for Non-SFI
  // mode.
  Variable *GotVar = nullptr;

  /// Randomize a given immediate operand
  Operand *randomizeOrPoolImmediate(Constant *Immediate,
                                    int32_t RegNum = Variable::NoRegister);
  X86OperandMem *
  randomizeOrPoolImmediate(X86OperandMem *MemOperand,
                           int32_t RegNum = Variable::NoRegister);
  bool RandomizationPoolingPaused = false;

private:
  /// dispatchToConcrete is the template voodoo that allows TargetX86Base to
  /// invoke methods in Machine (which inherits from TargetX86Base) without
  /// having to rely on virtual method calls. There are two overloads, one for
  /// non-void types, and one for void types. We need this becase, for non-void
  /// types, we need to return the method result, where as for void, we don't.
  /// While it is true that the code compiles without the void "version", there
  /// used to be a time when compilers would reject such code.
  ///
  /// This machinery is far from perfect. Note that, in particular, the
  /// arguments provided to dispatchToConcrete() need to match the arguments for
  /// Method **exactly** (i.e., no argument promotion is performed.)
  template <typename Ret, typename... Args>
  typename std::enable_if<!std::is_void<Ret>::value, Ret>::type
  dispatchToConcrete(Ret (ConcreteTarget::*Method)(Args...), Args &&... args) {
    return (static_cast<ConcreteTarget *>(this)->*Method)(
        std::forward<Args>(args)...);
  }

  template <typename... Args>
  void dispatchToConcrete(void (ConcreteTarget::*Method)(Args...),
                          Args &&... args) {
    (static_cast<ConcreteTarget *>(this)->*Method)(std::forward<Args>(args)...);
  }

  void lowerShift64(InstArithmetic::OpKind Op, Operand *Src0Lo, Operand *Src0Hi,
                    Operand *Src1Lo, Variable *DestLo, Variable *DestHi);

  /// Emit the code for a combined operation and consumer instruction, or set
  /// the destination variable of the operation if Consumer == nullptr.
  void lowerIcmpAndConsumer(const InstIcmp *Icmp, const Inst *Consumer);
  void lowerFcmpAndConsumer(const InstFcmp *Fcmp, const Inst *Consumer);
  void lowerArithAndConsumer(const InstArithmetic *Arith, const Inst *Consumer);

  /// Emit a setcc instruction if Consumer == nullptr; otherwise emit a
  /// specialized version of Consumer.
  void setccOrConsumer(BrCond Condition, Variable *Dest, const Inst *Consumer);

  /// Emit a mov [1|0] instruction if Consumer == nullptr; otherwise emit a
  /// specialized version of Consumer.
  void movOrConsumer(bool IcmpResult, Variable *Dest, const Inst *Consumer);

  /// Emit the code for instructions with a vector type.
  void lowerIcmpVector(const InstIcmp *Icmp);
  void lowerFcmpVector(const InstFcmp *Icmp);
  void lowerSelectVector(const InstSelect *Inst);

  /// Helpers for select lowering.
  void lowerSelectMove(Variable *Dest, BrCond Cond, Operand *SrcT,
                       Operand *SrcF);
  void lowerSelectIntMove(Variable *Dest, BrCond Cond, Operand *SrcT,
                          Operand *SrcF);
  /// Generic helper to move an arbitrary type from Src to Dest.
  void lowerMove(Variable *Dest, Operand *Src, bool IsRedefinition);

  /// Optimizations for idiom recognition.
  bool lowerOptimizeFcmpSelect(const InstFcmp *Fcmp, const InstSelect *Select);

  /// Emit code that initializes the value of the GotVar near the start of the
  /// function.  (This code is emitted only in Non-SFI mode.)
  void initGotVarIfNeeded();

  /// Complains loudly if invoked because the cpu can handle 64-bit types
  /// natively.
  template <typename T = Traits>
  typename std::enable_if<T::Is64Bit, void>::type lowerIcmp64(const InstIcmp *,
                                                              const Inst *) {
    llvm::report_fatal_error(
        "Hey, yo! This is x86-64. Watcha doin'? (lowerIcmp64)");
  }
  /// x86lowerIcmp64 handles 64-bit icmp lowering.
  template <typename T = Traits>
  typename std::enable_if<!T::Is64Bit, void>::type
  lowerIcmp64(const InstIcmp *Icmp, const Inst *Consumer);

  BoolFolding<Traits> FoldingInfo;

  static FixupKind PcRelFixup;
  static FixupKind AbsFixup;
};
} // end of namespace X86NAMESPACE
} // end of namespace Ice

#include "IceTargetLoweringX86BaseImpl.h"

#endif // SUBZERO_SRC_ICETARGETLOWERINGX86BASE_H
