#pragma once
#include "Register.h"
#include "OpCode.h"
#include "CondFlag.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Class describing an operand for an asm instruction. May be either:
	 * - a constant
	 * - a register
	 * - a pointer
	 * - a pointer relative a register
	 * - a label
	 * - a reference to something else in the Arena
	 * - a variable
	 * - a block
	 * - a condition flag
	 *
	 * Not all kinds of operands are valid for all kind of operations. Use the creator functions in
	 * 'Instruction.h' to create instructions, as these enforce the correct types.
	 */

	/**
	 * Type of the operand.
	 */
	enum OpType {
		// No data.
		opNone,

		// Constant.
		opConstant,

		// Dual-constant (one value for 32-bit and one for 64-bit). Appears as 'opConstant'.
		opDualConstant,

		// Register
		opRegister,

		// Relative to a register, ie. [reg + offset]
		opRelative,

		// Variable (+ offset).
		opVariable,

		// Label.
		opLabel,

		// Block or part.
		opBlock,

		// Reference to another object.
		opReference,

		// Condition flag.
		opCondFlag,
	};

	/**
	 * The operand itself.
	 */
	class Operand {
		STORM_VALUE;
	public:
		// No value.
		STORM_CTOR Operand();

		// Register.
		STORM_CAST_CTOR Operand(Register reg);

		// CondFlag
		STORM_CAST_CTOR Operand(CondFlag flag);

		/**
		 * The following constructors are mainly intended for use in the helpers below. They are
		 * public so I do not have to make all of them friend functions...
		 */

		// Create constants.
		Operand(Word c, Size size);
		Operand(Size s, Size size);
		Operand(Offset o, Size size);

		// Register + offset.
		Operand(Register r, Offset offset, Size size);
		// Operand(Variable v, Offset offset, Size size);

		/**
		 * Operations.
		 */

		// Compare.
		Bool STORM_FN operator ==(const Operand &o) const;
		Bool STORM_FN operator !=(const Operand &o) const;

		// Empty?
		Bool STORM_FN empty() const;

		// Type.
		OpType STORM_FN type() const;

		// Size.
		Size STORM_FN size() const;

		// Is this value readable and/or writable?
		Bool STORM_FN readable() const;
		Bool STORM_FN writable() const;

		// Throw an exception unless the value is readable/writable.
		void STORM_FN ensureReadable(op::Code op) const;
		void STORM_FN ensureWritable(op::Code op) const;

		/**
		 * Get values. Asserts if you try to get a value we're not representing.
		 */

		// Constant (depends on the backend).
		Word STORM_FN constant() const;
		Register STORM_FN reg() const; // "register" is a reserved word.
		Offset STORM_FN offset() const;
		CondFlag STORM_FN condFlag() const;

	private:
		// Our type.
		OpType opType;

		/**
		 * Storage. Optimized for small storage, while not using unions (as they are not supported by the CppTypes).
		 */

		// Store pointers here (ie. references, etc.)
		UNKNOWN(PTR_GC) void *opPtr;

		// Constant data/size data/offset data.
		Word opNum;

		// Offset (if needed).
		Offset opOffset;

		// Size of our data.
		Size opSize;
	};

	// Output.
	wostream &operator <<(wostream &to, const Operand &o);
	StrBuf &STORM_FN operator <<(StrBuf &to, Operand o);

	// Create constants.
	Operand STORM_FN byteConst(Byte v);
	Operand STORM_FN intConst(Int v);
	Operand STORM_FN natConst(Nat v);
	Operand STORM_FN longConst(Long v);
	Operand STORM_FN wordConst(Word v);
	Operand STORM_FN ptrConst(Size v);
	Operand STORM_FN ptrConst(Offset v);
	Operand STORM_FN xConst(Size s, Word v);

	// Relative values (dereference pointers).
	Operand STORM_FN byteRel(Register reg, Offset offset);
	Operand STORM_FN intRel(Register reg, Offset offset);
	Operand STORM_FN longRel(Register reg, Offset offset);
	Operand STORM_FN ptrRel(Register reg, Offset offset);
	Operand STORM_FN xRel(Size size, Register reg, Offset offset);

	// Access offsets inside variables (TODO)
	// Operand STORM_FN byteRel(Variable v, Offset offset);
	// Operand STORM_FN intRel(Variable v, Offset offset);
	// Operand STORM_FN longRel(Variable v, Offset offset);
	// Operand STORM_FN ptrRel(Variable v, Offset offset);
	// Operand STORM_FN xRel(Size size, Variable v, Offset offset);
}
