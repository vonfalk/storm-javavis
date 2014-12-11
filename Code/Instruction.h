#pragma once

#include "Arena.h"
#include "Value.h"
#include "OpCode.h"

namespace code {

	enum DestMode {
		destNone = 0x0,
		destRead = 0x1,
		destWrite = 0x2,
	};

	// Value type describing a single ASM instruction and its parameters.
	// Use the functions below to create op-code objects.
	class Instruction : public Printable {
	public:
		Instruction();


		inline const Value &src() const { return mySrc; }
		inline const Value &dest() const { return myDest; }
		inline DestMode destMode() const { return myDestMode; }
		inline OpCode op() const { return opCode; }

		// Get the maxium size of operands needed.
		nat size() const;

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

	inline DestMode operator |(DestMode a, DestMode b) { return DestMode(int(a) | int(b)); }
	inline DestMode operator &(DestMode a, DestMode b) { return DestMode(int(a) & int(b)); }

	// Conditional for jumps and others.
	enum CondFlag {
		ifAlways,
		ifNever,

		ifOverflow,
		ifNoOverflow,
		ifEqual,
		ifNotEqual,

		// Unsigned comparision.
		ifBelow,
		ifBelowEqual,
		ifAboveEqual,
		ifAbove,

		// Singned comparision.
		ifLess,
		ifLessEqual,
		ifGreaterEqual,
		ifGreater,
	};

	// Get the string name.
	const wchar_t *name(CondFlag cond);

	// Inverse the flag.
	CondFlag inverse(CondFlag cond);

	// Functions for creating Instruction objects.
	Instruction mov(const Value &to, const Value &from);
	Instruction push(const Value &v);
	Instruction pop(const Value &to);
	Instruction jmp(const Value &to, CondFlag cond = ifAlways);
	Instruction call(const Value &to, nat returnSize);
	Instruction ret(nat returnSize);

	// Set a byte to the result of an operation.
	Instruction setCond(const Value &to, CondFlag cond);

	// Function calls. Works like this: fnParam(10), fnParam(20), fnCall(myFn). Labels placed between fnParam
	// and fnCall will effectively be before the first fnParam. All fnParams will be merged into the fnCall, so do
	// not place other instructions between. Parameters are entered "right to left". Ie, the above example
	// calls myFn(10, 20).
	Instruction fnParam(const Value &src);
	Instruction fnCall(const Value &src, nat returnSize);

	// Integer math (signed)
	Instruction add(const Value &dest, const Value &src);
	Instruction adc(const Value &dest, const Value &src);
	Instruction or(const Value &dest, const Value &src);
	Instruction and(const Value &dest, const Value &src);
	Instruction sub(const Value &dest, const Value &src);
	Instruction sbb(const Value &dest, const Value &src);
	Instruction xor(const Value &dest, const Value &src);
	Instruction cmp(const Value &dest, const Value &src);
	Instruction mul(const Value &dest, const Value &src);

	// Shifts. Src.size() == 1
	Instruction shl(const Value &dest, const Value &src);
	Instruction shr(const Value &dest, const Value &src);
	Instruction sar(const Value &dest, const Value &src);

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
	Instruction begin(Block block);
	Instruction end(Block block);

	// Segment override for the next instruction, not reliable for function calls or double memory accesses. Only
	// availiable on relevant architectures, for example X86.
	Instruction threadLocal();
}
