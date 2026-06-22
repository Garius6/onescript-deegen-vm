// OneScript bytecode definitions in Deegen DSL.
//
// This file is processed at *build time* by the Deegen meta-compiler. It does
// not compile as ordinary C++ — see third_party/deegen/annotated/bytecodes/
// for the reference setup that we mimic. Deegen scans these definitions and
// emits both an interpreter and a baseline JIT for the declared opcodes.
//
// At this stage we hand-author a register-based opcode set that maps to a
// subset of our stack-based .osbin bytecode. A separate translator (planned
// in docs/deegen-integration-plan.md, Path B) will lower OneScript's stack
// semantics into these register-based forms during module loading.

#include "api_define_bytecode.h"
#include "deegen_api.h"

#include "runtime_utils.h"

// -----------------------------------------------------------------------------
// OsLoadInt16 — load a signed int16 literal into the destination slot.
//   Equivalent OneScript stack op: PushInt N
// -----------------------------------------------------------------------------

static void NO_RETURN OsLoadInt16Impl(int16_t value)
{
    Return(TValue::Create<tDouble>(value));
}

DEEGEN_DEFINE_BYTECODE(OsLoadInt16)
{
    Operands(Literal<int16_t>("value"));
    Result(BytecodeValue);
    Implementation(OsLoadInt16Impl);
    Variant();
    DfgVariant();
    TypeDeductionRule(AlwaysOutput<tDouble>);
    RegAllocHint(Op("output").RegHint(RegHint::FPR));
}

// -----------------------------------------------------------------------------
// OsMov — copy from one slot/constant into the destination slot.
//   Equivalent OneScript stack op pair: PushLoc src; LoadLoc dst  (after
//   stack-to-register lowering this collapses to a register move).
// -----------------------------------------------------------------------------

static void NO_RETURN OsMovImpl(TValue input)
{
    Return(input);
}

DEEGEN_DEFINE_BYTECODE(OsMov)
{
    Operands(BytecodeSlotOrConstant("input"));
    Result(BytecodeValue);
    Implementation(OsMovImpl);
    Variant(Op("input").IsBytecodeSlot());
    Variant(Op("input").IsConstant());
    DfgVariant();
    TypeDeductionRule([](TypeMask input) { return input; });
}

// -----------------------------------------------------------------------------
// OsAdd / OsSub — binary arithmetic on doubles. OneScript types reduce to
// double for compute-heavy paths (decimal is reserved for upstream-compat
// scenarios that are out of scope for the PoC).
//   Equivalent OneScript stack op: ... Add  /  ... Sub
// -----------------------------------------------------------------------------

enum class OsArithKind
{
    Add,
    Sub,
};

template<OsArithKind opKind>
static void NO_RETURN OsArithmeticImpl(TValue lhs, TValue rhs)
{
    if (likely(lhs.Is<tDouble>() && rhs.Is<tDouble>()))
    {
        double ld = lhs.As<tDouble>();
        double rd = rhs.As<tDouble>();
        double res;
        if constexpr (opKind == OsArithKind::Add)
        {
            res = ld + rd;
        }
        else
        {
            static_assert(opKind == OsArithKind::Sub, "unexpected opKind");
            res = ld - rd;
        }
        Return(TValue::Create<tDouble>(res));
    }
    else
    {
        // PoC error path: upstream OneScript throws "Wrong type for arithmetic operation".
        // We surface a simple error for now; richer messages can come later.
        ThrowError("OneScript arithmetic requires numeric operands");
    }
}

DEEGEN_DEFINE_BYTECODE_TEMPLATE(OsArithmetic, OsArithKind opKind)
{
    Operands(
        BytecodeSlotOrConstant("lhs"),
        BytecodeSlotOrConstant("rhs"));
    Result(BytecodeValue);
    Implementation(OsArithmeticImpl<opKind>);
    Variant(
        Op("lhs").IsBytecodeSlot(),
        Op("rhs").IsBytecodeSlot());
    Variant(
        Op("lhs").IsConstant<tDoubleNotNaN>(),
        Op("rhs").IsBytecodeSlot());
    Variant(
        Op("lhs").IsBytecodeSlot(),
        Op("rhs").IsConstant<tDoubleNotNaN>());
    DfgVariant(
        Op("lhs").HasType<tDouble>(),
        Op("rhs").HasType<tDouble>());
    DfgVariant();
    TypeDeductionRule(
        [](TypeMask lhs, TypeMask rhs) -> TypeMask {
            if (lhs.SubsetOf<tDouble>() && rhs.SubsetOf<tDouble>())
            {
                return x_typeMaskFor<tDouble>;
            }
            return x_typeMaskFor<tBoxedValueTop>;
        });
    RegAllocHint(
        Op("lhs").RegHint(RegHint::FPR),
        Op("rhs").RegHint(RegHint::FPR),
        Op("output").RegHint(RegHint::FPR));
}

DEEGEN_DEFINE_BYTECODE_BY_TEMPLATE_INSTANTIATION(OsAdd, OsArithmetic, OsArithKind::Add);
DEEGEN_DEFINE_BYTECODE_BY_TEMPLATE_INSTANTIATION(OsSub, OsArithmetic, OsArithKind::Sub);

// -----------------------------------------------------------------------------
// OsRet — return a single value from the current OneScript method.
//   Equivalent OneScript stack op: Return (with the value on top of stack).
// -----------------------------------------------------------------------------

static void NO_RETURN OsRetImpl(const TValue* retStart, uint16_t numRet)
{
    GuestLanguageFunctionReturn(retStart, numRet);
}

DEEGEN_DEFINE_BYTECODE(OsRet)
{
    Operands(
        BytecodeRangeBaseRO("retStart"),
        Literal<uint16_t>("numRet"));
    Result(NoOutput);
    Implementation(OsRetImpl);
    Variant(Op("numRet").HasValue(0));   // procedure return — no values
    Variant(Op("numRet").HasValue(1));   // single function value (common case)
    Variant();
    DfgVariant();
    DeclareReads(Range(Op("retStart"), Op("numRet")));
    DeclareAsIntrinsic<Intrinsic::FunctionReturn>({
        .start = Op("retStart"),
        .length = Op("numRet"),
    });
}

DEEGEN_END_BYTECODE_DEFINITIONS
