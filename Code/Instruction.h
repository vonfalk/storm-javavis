#pragma once

#include "Arena.h"
#include "Value.h"
#include "OpCode.h"
#include "Utils/Bitmask.h"

namespace code {

	/**
	 * Type for a return value, as we need to know if the value is a float or not.
	 */
	struct RetVal {
		Size size;
		bool isFloat;
	};

	inline RetVal retVal(const Size &size, bool isFloat) {
		RetVal r = { size, isFloat };
		return r;
	}

	inline RetVal retVoid() {
		return retVal(Size(), false);
	}

	inline RetVal retPtr() {
		return retVal(Size::sPtr, false);
	}

	/**
	 * What is done to the dest field of an instruction?
	 */
	enum DestMode {
		destNone = 0x0,
		destRead = 0x1,
		destWrite = 0x2,
	};

	/**
	 * Value type describing a single ASM instruction and its parameters.
	 * Use the functions below to create op-code objects.
	 */
	class Instruction : public Printable {
	public:
		Instruction();


		inline const Value &src() const { return mySrc; }
		inline const Value &dest() const { return myDest; }
		inline DestMode destMode() const { return myDestMode; }
		inline OpCode op() const { return opCode; }

		// Get the maxium size of operands needed for the current cpu.
		nat currentSize() const;
		Size size() const;

		// These are intended for use by the backends, no sanity checks are made here!
		Instruction altered(const Value &dest, const Value &src) const;
		Instruction alterSrc(const Value &v) const;
		Instruction alterDest(const Value &v) const;
	protected:
		void output(std::wostream &to) const;

	private:
		Instruction(OpCode opCode, const Value &dest, DestMode destMode, const Value &src);

		OpCode opCode;

		// The source and destination operand. Both are not always valid for all op-codes.
		// "src" is always read from, never written to
		// "dest" is written to, might be read from too.
		Value myDest, mySrc;

		// Reading and/or writing to dest?
		DestMode myDestMode;

		friend Instruction create(OpCode opCode);
		friend Instruction createSrc(OpCode opCode, const Value &src);
		friend Instruction createDest(OpCode opCode, const Value &dest, DestMode mode);
		friend Instruction createDestSrc(OpCode opCode, const Value &dest, DestMode destMode, const Value &src);
		friend Instruction createLoose(OpCode opCode, const Value &dest, DestMode destMode, const Value &src);
	};

	BITMASK_OPERATORS(DestMode);

	// Functions for creating Instruction objects.
	Instruction mov(const Value &to, const Value &from);
	Instruction push(const Value &v);
	Instruction pop(const Value &to);
	Instruction jmp(const Value &to, CondFlag cond = ifAlways);
	Instruction call(const Value &to, RetVal ret);
	Instruction ret(RetVal ret); // Returns whatever is in eax register.

	// This one has somewhat special semantics, when used with a reference as 'from', it loads a representative
	// value that can be passed to 'Ref::fromLea' to re-create the reference.
	Instruction lea(const Value &to, const Value &from);

	// Set a byte to the result of an operation.
	Instruction setCond(const Value &to, CondFlag cond);

	// Function calls. Works like this: fnParam(10), fnParam(20), fnCall(myFn). Labels placed between fnParam
	// and fnCall will effectively be before the first fnParam. All fnParams will be merged into the fnCall, so do
	// not place other instructions between. Parameters are entered "right to left". Ie, the above example
	// calls myFn(10, 20).
	// fnParam taking two parameters uses the function 'copy' to copy 'src' onto the stack. 'copy' is assumed
	// to have the signature <ptr or void> copy(void *dest, void *src), like copy ctors in C++.
	// fnParamRef is like the two-parameter version of fnParam, except that it dereferences the pointer at 'src' and
	// uses that value instead of something contained inside a variable.
	Instruction fnParam(const Value &src);
	Instruction fnParam(const Variable &src, const Value &copyFn);
	Instruction fnParamRef(const Value &src, const Value &copyFn);
	Instruction fnCall(const Value &src, RetVal ret);

	// Integer math (signed/unsigned)
	Instruction add(const Value &dest, const Value &src);
	Instruction adc(const Value &dest, const Value &src);
	Instruction or(const Value &dest, const Value &src);
	Instruction and(const Value &dest, const Value &src);
	Instruction sub(const Value &dest, const Value &src);
	Instruction sbb(const Value &dest, const Value &src);
	Instruction xor(const Value &dest, const Value &src);
	Instruction cmp(const Value &dest, const Value &src);
	Instruction mul(const Value &dest, const Value &src);

	// Signed math
	Instruction idiv(const Value &dest, const Value &src); // div a, b <=> a = a / b
	Instruction imod(const Value &dest, const Value &src);

	// Unsigned math
	Instruction udiv(const Value &dest, const Value &src);
	Instruction umod(const Value &dest, const Value &src);

	// Shifts. Src.size() == 1
	Instruction shl(const Value &dest, const Value &src);
	Instruction shr(const Value &dest, const Value &src);
	Instruction sar(const Value &dest, const Value &src);

	// Resize operands. Works like a mov, but sign-extends if needed.
	Instruction icast(const Value &dest, const Value &src);
	Instruction ucast(const Value &dest, const Value &src);

	// Floating point math.
	Instruction fstp(const Value &dest);
	Instruction fistp(const Value &dest); // Truncates results.
	Instruction fld(const Value &src);
	Instruction fild(const Value &src);
	Instruction faddp();
	Instruction fsubp();
	Instruction fmulp();
	Instruction fdivp();
	Instruction fcompp();
	Instruction fwait();

	// Data
	Instruction dat(const Value &v);

	// Reference counting.
	Instruction addRef(const Value &to);
	Instruction releaseRef(const Value &to);

	// Function prolog/epilog. The prolog will automatically initialize the root block, and the epilog
	// will automatically destroy all active blocks. The epilog does not count as closing a block, and
	// may therefore be used inside functions to implement a return statement. For example:
	// listing << prolog() << begin(block2) << epilog() << ret() << ... << end(block2) << ... << epilog() << ret();
	// Here: block2 is considered to continue all the way between begin() and end(), even though the block is
	// also ended within the epilog in the middle. The epilog only preserves ptrA register.
	Instruction prolog();
	Instruction epilog();

	// Scope management. A scope is assumed to be open on all instructions between "begin(scope)" and "end(scope)".
	// Do not, for example, have all begin/end scopes in the end of the listing and jump forth and back between them.
	// That makes the exception handling fail to detect what to destroy. NOTE: Does not preserve registers!
	Instruction begin(Part block);
	Instruction end(Part block);

	// Segment override for the next instruction, not reliable for function calls or double memory accesses. Only
	// availiable on relevant architectures, for example X86.
	Instruction threadLocal();
}
