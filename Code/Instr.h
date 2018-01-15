#pragma once
#include "OpCode.h"
#include "Operand.h"
#include "CondFlag.h"
#include "TypeDesc.h"
#include "Core/Object.h"
#include "Core/EnginePtr.h"
#include "Utils/Bitmask.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Class representing an entire asm-instruction.
	 *
	 * Note: immutable class.
	 */
	class Instr : public Object {
		STORM_CLASS;
	public:
		// Create a no-op instruction.
		STORM_CTOR Instr();

		// Copy this instruction.
		Instr(const Instr &o);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Access to the members:
		op::Code STORM_FN op() const;
		Operand STORM_FN src() const;
		Operand STORM_FN dest() const;
		DestMode STORM_FN mode() const;

		// Get the maximum size of the operands.
		Size STORM_FN size() const;

		// Create another instruction based off this one. Intended to be used by backends -> no sanity checking.
		virtual Instr *STORM_FN alter(Operand dest, Operand src);
		virtual Instr *STORM_FN alterSrc(Operand src);
		virtual Instr *STORM_FN alterDest(Operand dest);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

	protected:
		// Private constructor, used by 'instrXxx()' functions below.
		Instr(op::Code opCode, const Operand &dest, const Operand &src);

		// Op-code.
		op::Code iOp;

		// Src and dest fields.
		Operand iSrc;
		Operand iDest;

		// Creators.
		friend Instr *instr(EnginePtr e, op::Code op);
		friend Instr *instrSrc(EnginePtr e, op::Code op, Operand src);
		friend Instr *instrDest(EnginePtr e, op::Code op, Operand dest);
		friend Instr *instrDestSrc(EnginePtr e, op::Code op, Operand dest, Operand src);
		friend Instr *instrLoose(EnginePtr e, op::Code op, Operand dest, Operand src);
	};

	/**
	 * Additional information required for FnCall and FnRet operations. Contains an additional
	 * 'TypeDesc' describing the value being used in detail.
	 *
	 * TODO: If we wanted to, we could probably squeeze the extra pointer inside the Operand in a
	 * regular Instr somehow. I do not, however, think the effort is worth it.
	 */
	class TypeInstr : public Instr {
		STORM_CLASS;
	public:
		// Create.
		TypeInstr(op::Code opCode, const Operand &dest, const Operand &src, TypeDesc *type, Bool member);

		// The type of used in this instruction.
		TypeDesc *type;

		// Is this a call to a member function? Only used for 'fnCall' instructions.
		Bool member;

		// Alter various parts.
		virtual Instr *STORM_FN alter(Operand dest, Operand src);
		virtual Instr *STORM_FN alterSrc(Operand src);
		virtual Instr *STORM_FN alterDest(Operand dest);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;
	};

	// Create an instruction without operands.
	Instr *STORM_FN instr(EnginePtr e, op::Code op);
	Instr *STORM_FN instrSrc(EnginePtr e, op::Code op, Operand src);
	Instr *STORM_FN instrDest(EnginePtr e, op::Code op, Operand dest);
	Instr *STORM_FN instrDestSrc(EnginePtr e, op::Code op, Operand dest, Operand src);

	// Create without checking the sanity of the parameters. Used for pseudo-instructions and predicates among others.
	Instr *STORM_FN instrLoose(EnginePtr e, op::Code op, Operand dest, Operand src);

	/**
	 * Create instructions:
	 */

	Instr *STORM_FN mov(EnginePtr e, Operand to, Operand from);
	Instr *STORM_FN swap(EnginePtr e, Reg a, Operand b);
	Instr *STORM_FN jmp(EnginePtr e, Operand to);
	Instr *STORM_FN jmp(EnginePtr e, Label to, CondFlag cond);
	Instr *STORM_FN call(EnginePtr e, Operand to, Size resultSize);
	Instr *STORM_FN ret(EnginePtr e, Size resultSize); // Returns whatever is in eax register.

	// Note: Be very careful when using push and pop in code that performs function calls! Certain
	// architectures require the stack to be properly aligned when calling a function. The different
	// backends takes care of this for you, but they will not keep track of push and pop
	// instructions so that they can compensate for any additional instructions used.
	Instr *STORM_FN push(EnginePtr e, Operand v);
	Instr *STORM_FN pop(EnginePtr e, Operand to);
	Instr *STORM_FN pushFlags(EnginePtr e);
	Instr *STORM_FN popFlags(EnginePtr e);

	// This one has somewhat special semantics, when used with a reference as 'from', it instead
	// loads the RefSource referred to by the Ref.
	Instr *STORM_FN lea(EnginePtr e, Operand to, Operand from);

	// Set a byte to the result of an operation.
	Instr *STORM_FN setCond(EnginePtr e, Operand to, CondFlag cond);

	// Function calls. Works like this: fnParam(10), fnParam(20), fnCall(myFn). Labels placed between fnParam
	// and fnCall will effectively be before the first fnParam. All fnParams will be merged into the fnCall, so do
	// not place other instructions between. Parameters are entered "right to left". Ie, the above example
	// calls myFn(10, 20).
	// The variations ending in 'Ref' assumes that the source (or destination) is a pointer rather than
	// the target itself.
	Instr *STORM_FN fnParam(EnginePtr e, TypeDesc *type, Operand src);
	Instr *STORM_FN fnParamRef(EnginePtr e, TypeDesc *type, Operand src);
	Instr *STORM_FN fnCall(EnginePtr e, Operand call, Bool member); // no result
	Instr *STORM_FN fnCall(EnginePtr e, Operand call, Bool member, TypeDesc *result, Operand to);
	Instr *STORM_FN fnCallRef(EnginePtr e, Operand call, Bool member, TypeDesc *result, Operand to);

	// High-level function return. Examines the return type specified in the listing and handles the
	// return properly according to the current platform. Implies an 'epilog' instruction as well.
	Instr *STORM_FN fnRet(EnginePtr e, Operand src);
	Instr *STORM_FN fnRetRef(EnginePtr e, Operand src);
	Instr *STORM_FN fnRet(EnginePtr e);

	// Integer math (signed/unsigned)
	Instr *STORM_FN STORM_NAME(bor, or)(EnginePtr e, Operand dest, Operand src);
	Instr *STORM_FN STORM_NAME(band, and)(EnginePtr e, Operand dest, Operand src);
	Instr *STORM_FN STORM_NAME(bxor, xor)(EnginePtr e, Operand dest, Operand src);
	Instr *STORM_FN STORM_NAME(bnot, not)(EnginePtr e, Operand dest);
	Instr *STORM_FN add(EnginePtr e, Operand dest, Operand src);
	Instr *STORM_FN adc(EnginePtr e, Operand dest, Operand src);
	Instr *STORM_FN sub(EnginePtr e, Operand dest, Operand src);
	Instr *STORM_FN sbb(EnginePtr e, Operand dest, Operand src);
	Instr *STORM_FN cmp(EnginePtr e, Operand dest, Operand src);
	Instr *STORM_FN mul(EnginePtr e, Operand dest, Operand src);

	// Signed math
	Instr *STORM_FN idiv(EnginePtr e, Operand dest, Operand src); // div a, b <=> a = a / b
	Instr *STORM_FN imod(EnginePtr e, Operand dest, Operand src);

	// Unsigned math
	Instr *STORM_FN udiv(EnginePtr e, Operand dest, Operand src);
	Instr *STORM_FN umod(EnginePtr e, Operand dest, Operand src);

	// Shifts. src.size() == 1
	Instr *STORM_FN shl(EnginePtr e, Operand dest, Operand src);
	Instr *STORM_FN shr(EnginePtr e, Operand dest, Operand src);
	Instr *STORM_FN sar(EnginePtr e, Operand dest, Operand src);

	// Resize operands. Works like a mov, but sign-extends if needed.
	Instr *STORM_FN icast(EnginePtr e, Operand dest, Operand src);
	Instr *STORM_FN ucast(EnginePtr e, Operand dest, Operand src);

	// Floating point math.
	Instr *STORM_FN fstp(EnginePtr e, Operand dest);
	Instr *STORM_FN fistp(EnginePtr e, Operand dest); // Truncates results.
	Instr *STORM_FN fld(EnginePtr e, Operand src);
	Instr *STORM_FN fild(EnginePtr e, Operand src);
	Instr *STORM_FN faddp(EnginePtr e);
	Instr *STORM_FN fsubp(EnginePtr e);
	Instr *STORM_FN fmulp(EnginePtr e);
	Instr *STORM_FN fdivp(EnginePtr e);
	Instr *STORM_FN fcompp(EnginePtr e);
	Instr *STORM_FN fwait(EnginePtr e);

	// Data.
	Instr *STORM_FN dat(EnginePtr e, Operand v);

	// Offset of a label.
	Instr *STORM_FN lblOffset(EnginePtr e, Label l);

	// Align to specific size.
	Instr *STORM_FN alignAs(EnginePtr e, Size v);
	Instr *STORM_FN align(EnginePtr e, Offset v);

	// Function prolog/epilog. The prolog will automatically initialize the root block, and the epilog
	// will automatically destroy all active blocks. The epilog does not count as closing a block, and
	// may therefore be used inside functions to implement a return statement. For example:
	// listing << prolog() << begin(block2) << epilog() << ret() << ... << end(block2) << ... << epilog() << ret();
	// Here: block2 is considered to continue all the way between begin() and end(), even though the block is
	// also ended within the epilog in the middle. The epilog only preserves ptrA register.
	Instr *STORM_FN prolog(EnginePtr e);
	Instr *STORM_FN epilog(EnginePtr e);

	// Note that a register has been preserved on the stack.
	Instr *STORM_FN preserve(EnginePtr e, Operand dest, Reg reg);

	// Scope management. A scope is assumed to be open on all instructions between "begin(scope)" and "end(scope)".
	// Do not, for example, have all begin/end scopes in the end of the listing and jump forth and back between them.
	// That makes the exception handling fail to detect what to destroy. NOTE: Does not preserve registers!
	Instr *STORM_FN begin(EnginePtr e, Part part);
	Instr *STORM_FN end(EnginePtr e, Part part);

	// Segment override for the next instruction, not reliable for function calls or double memory accesses. Only
	// availiable on relevant architectures, for example X86.
	Instr *STORM_FN threadLocal(EnginePtr e);

	// Note: add a forwarder in InstrFwd as well!
}
