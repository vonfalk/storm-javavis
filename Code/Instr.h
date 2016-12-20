#pragma once
#include "OpCode.h"
#include "Operand.h"
#include "CondFlag.h"
#include "ValType.h"
#include "Core/Object.h"
#include "Core/EnginePtr.h"
#include "Utils/Bitmask.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Value representing an entire asm-instruction.
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
		Instr *STORM_FN alter(Operand dest, Operand src);
		Instr *STORM_FN alterSrc(Operand src);
		Instr *STORM_FN alterDest(Operand dest);

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
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
	Instr *STORM_FN push(EnginePtr e, Operand v);
	Instr *STORM_FN pop(EnginePtr e, Operand to);
	Instr *STORM_FN pushFlags(EnginePtr e);
	Instr *STORM_FN popFlags(EnginePtr e);
	Instr *STORM_FN jmp(EnginePtr e, Operand to);
	Instr *STORM_FN jmp(EnginePtr e, Operand to, CondFlag cond);
	Instr *STORM_FN call(EnginePtr e, Operand to, ValType ret);
	Instr *STORM_FN ret(EnginePtr e, ValType ret); // Returns whatever is in eax register.

	// This one has somewhat special semantics, when used with a reference as 'from', it loads a representative
	// value that can be passed to 'Ref::fromLea' to re-create the reference.
	Instr *STORM_FN lea(EnginePtr e, Operand to, Operand from);

	// Set a byte to the result of an operation.
	Instr *STORM_FN setCond(EnginePtr e, Operand to, CondFlag cond);

	// Function calls. Works like this: fnParam(10), fnParam(20), fnCall(myFn). Labels placed between fnParam
	// and fnCall will effectively be before the first fnParam. All fnParams will be merged into the fnCall, so do
	// not place other instructions between. Parameters are entered "right to left". Ie, the above example
	// calls myFn(10, 20).
	// fnParam taking two parameters uses the function 'copy' to copy 'src' onto the stack. 'copy' is assumed
	// to have the signature <ptr or void> copy(void *dest, void *src), like copy ctors in C++.
	// Having an empty 'copyFn' to 'fnParam' will generate a 'memcpy'-like copy.
	Instr *STORM_FN fnParam(EnginePtr e, Operand src);
	Instr *STORM_FN fnParam(EnginePtr e, Var src, Operand copyFn);
	Instr *STORM_FN fnParamRef(EnginePtr e, Operand src, Size size);
	Instr *STORM_FN fnParamRef(EnginePtr e, Operand src, Size size, Operand copyFn);
	Instr *STORM_FN fnCall(EnginePtr e, Operand src, ValType ret);

	// Integer math (signed/unsigned)
	Instr *STORM_FN or(EnginePtr e, Operand dest, Operand src);
	Instr *STORM_FN and(EnginePtr e, Operand dest, Operand src);
	Instr *STORM_FN xor(EnginePtr e, Operand dest, Operand src);
	Instr *STORM_FN not(EnginePtr e, Operand dest);
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

	// Function prolog/epilog. The prolog will automatically initialize the root block, and the epilog
	// will automatically destroy all active blocks. The epilog does not count as closing a block, and
	// may therefore be used inside functions to implement a return statement. For example:
	// listing << prolog() << begin(block2) << epilog() << ret() << ... << end(block2) << ... << epilog() << ret();
	// Here: block2 is considered to continue all the way between begin() and end(), even though the block is
	// also ended within the epilog in the middle. The epilog only preserves ptrA register.
	Instr *STORM_FN prolog(EnginePtr e);
	Instr *STORM_FN epilog(EnginePtr e);

	// Scope management. A scope is assumed to be open on all instructions between "begin(scope)" and "end(scope)".
	// Do not, for example, have all begin/end scopes in the end of the listing and jump forth and back between them.
	// That makes the exception handling fail to detect what to destroy. NOTE: Does not preserve registers!
	Instr *STORM_FN begin(EnginePtr e, Part part);
	Instr *STORM_FN end(EnginePtr e, Block block);

	// Segment override for the next instruction, not reliable for function calls or double memory accesses. Only
	// availiable on relevant architectures, for example X86.
	Instr *STORM_FN threadLocal(EnginePtr e);


	/**
	 * Make these functions more convenient to call in C++ during certain conditions:
	 *
	 * It inhibits the clarity and it is a bit annoying to have to do:
	 * *foo << mov(e, eax, ebx);
	 *
	 * instead of:
	 * *foo << mov(eax, ebx);
	 *
	 * The first engine parameter is not visible in Storm (as it will insert that for us
	 * automatically), but in C++ we need another mechanism as C++ does not know which engine we
	 * will use. So in the context of directly appending instructions to a Listing object, we can
	 * save the parameters in some container temporarily and delay the call to the actual creating
	 * function until the result is appended to a Listing. When this happens, we can get an Engine
	 * from there and create the Instr object properly. This is done with the template magic
	 * below. Sadly, we need to declare all machine operations once more at the end, but that is a
	 * small price to pay.
	 */

	class Listing;

	template <Instr *(*Fn)(EnginePtr)>
	class InstrProxy0 {};

	template <Instr *(*Fn)(EnginePtr)>
	inline Listing &operator <<(Listing &l, const InstrProxy0<Fn> &p) {
		return l << (*Fn)(l.engine());
	}


	template <class T, Instr *(*Fn)(EnginePtr, T)>
	class InstrProxy1 {
	public:
		const T &t;
		InstrProxy1(const T &t) : t(t) {}
	};

	template <class T, Instr *(*Fn)(EnginePtr, T)>
	inline Listing &operator <<(Listing &l, const InstrProxy1<T, Fn> &p) {
		return l << (*Fn)(l.engine(), p.t);
	}


	template <class T, class U, Instr *(*Fn)(EnginePtr, T, U)>
	class InstrProxy2 {
	public:
		const T &t;
		const U &u;
		InstrProxy2(const T &t, const U &u) : t(t), u(u) {}
	};

	template <class T, class U, Instr *(*Fn)(EnginePtr, T, U)>
	inline Listing &operator <<(Listing &l, const InstrProxy2<T, U, Fn> &p) {
		return l << (*Fn)(l.engine(), p.t, p.u);
	}


	template <class T, class U, class V, Instr *(*Fn)(EnginePtr, T, U, V)>
	class InstrProxy3 {
	public:
		const T &t;
		const U &u;
		const V &v;
		InstrProxy3(const T &t, const U &u, const V &v) : t(t), u(u), v(v) {}
	};

	template <class T, class U, class V, Instr *(*Fn)(EnginePtr, T, U, V)>
	inline Listing &operator <<(Listing &l, const InstrProxy3<T, U, V, Fn> &p) {
		return l << (*Fn)(l.engine(), p.t, p.u, p.v);
	}


#define PROXY0(op)												\
	inline InstrProxy0<&op> op() { return InstrProxy0<&op>(); }
#define PROXY1(op, T)													\
	inline InstrProxy1<T, &op> op(const T &t) { return InstrProxy1<T, &op>(t); }
#define PROXY2(op, T, U)												\
	inline InstrProxy2<T, U, &op> op(const T &t, const U &u) { return InstrProxy2<T, U, &op>(t, u); }
#define PROXY3(op, T, U, V)												\
	inline InstrProxy3<T, U, V, &op> op(const T &t, const U &u, const V &v) { return InstrProxy3<T, U, V, &op>(t, u, v); }


	// Repetition of all OP-codes.
	PROXY2(mov, Operand, Operand);
	PROXY1(push, Operand);
	PROXY1(pop, Operand);
	PROXY0(pushFlags);
	PROXY0(popFlags);
	PROXY1(jmp, Operand);
	PROXY2(jmp, Operand, CondFlag);
	PROXY2(call, Operand, ValType);
	PROXY1(ret, ValType);
	PROXY2(lea, Operand, Operand);
	PROXY2(setCond, Operand, CondFlag);
	PROXY1(fnParam, Operand);
	PROXY2(fnParam, Var, Operand);
	PROXY2(fnParamRef, Operand, Size);
	PROXY3(fnParamRef, Operand, Size, Operand);
	PROXY2(fnCall, Operand, ValType);
	PROXY2(or, Operand, Operand);
	PROXY2(and, Operand, Operand);
	PROXY2(xor, Operand, Operand);
	PROXY1(not, Operand);
	PROXY2(add, Operand, Operand);
	PROXY2(adc, Operand, Operand);
	PROXY2(sub, Operand, Operand);
	PROXY2(sbb, Operand, Operand);
	PROXY2(cmp, Operand, Operand);
	PROXY2(mul, Operand, Operand);
	PROXY2(idiv, Operand, Operand);
	PROXY2(imod, Operand, Operand);
	PROXY2(udiv, Operand, Operand);
	PROXY2(umod, Operand, Operand);
	PROXY2(shl, Operand, Operand);
	PROXY2(shr, Operand, Operand);
	PROXY2(sar, Operand, Operand);
	PROXY2(icast, Operand, Operand);
	PROXY2(ucast, Operand, Operand);
	PROXY1(fstp, Operand);
	PROXY1(fistp, Operand);
	PROXY1(fld, Operand);
	PROXY1(fild, Operand);
	PROXY0(faddp);
	PROXY0(fsubp);
	PROXY0(fmulp);
	PROXY0(fdivp);
	PROXY0(fcompp);
	PROXY0(fwait);
	PROXY1(dat, Operand);
	PROXY0(prolog);
	PROXY0(epilog);
	PROXY1(begin, Part);
	PROXY1(end, Block);
	PROXY0(threadLocal);

}
