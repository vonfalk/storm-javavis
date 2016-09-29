#pragma once
#include "Register.h"
#include "OpCode.h"
#include "CondFlag.h"
#include "Variable.h"
#include "Block.h"
#include "Label.h"
#include "Reference.h"

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

		// Dual-constant (one value for 32-bit and one for 64-bit). Appears as 'opConstant', but is stored in 'opOffset'.
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
		opPart,

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

		// CondFlag.
		STORM_CAST_CTOR Operand(CondFlag flag);

		// Variable.
		STORM_CAST_CTOR Operand(Variable var);

		// Block or part.
		STORM_CAST_CTOR Operand(Part part);

		// Label.
		STORM_CAST_CTOR Operand(Label label);

		// Reference.
		STORM_CAST_CTOR Operand(Ref ref);
		STORM_CAST_CTOR Operand(Reference *ref);

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
		Part STORM_FN part() const;
		Label STORM_FN label() const;
		Ref STORM_FN ref() const;
		Variable STORM_FN variable() const; // NOTE: The size of this variable is equal to the size
											// we want to read, which is not always the size of the
											// original variable!

		/**
		 * Debug.
		 */

		void dbg_dump() const;

	private:
		// Create constants.
		Operand(Word c, Size size);
		Operand(Size s, Size size);
		Operand(Offset o, Size size);

		// Register + offset.
		Operand(Register r, Offset offset, Size size);
		Operand(Variable v, Offset offset, Size size);

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

		// Friends.
		friend Operand ptrConst(Size v);
		friend Operand ptrConst(Offset v);
		friend Operand xConst(Size s, Word w);
		friend Operand xRel(Size size, Register reg, Offset offset);
		friend Operand xRel(Size size, Variable v, Offset offset);
		friend wostream &operator <<(wostream &to, const Operand &o);
	};

	// Output.
	wostream &operator <<(wostream &to, const Operand &o);
	StrBuf &STORM_FN operator <<(StrBuf &to, Operand o);

	// Create constants. Note: Do not try to create a pointer to a Gc:d object using xConst. That
	// will fail miserably, as the pointer will not be updated whenever the Gc moves the object.
	Operand STORM_FN byteConst(Byte v);
	Operand STORM_FN intConst(Int v);
	Operand STORM_FN natConst(Nat v);
	Operand STORM_FN longConst(Long v);
	Operand STORM_FN wordConst(Word v);
	Operand STORM_FN floatConst(Float v);
	Operand STORM_FN ptrConst(Size v);
	Operand STORM_FN ptrConst(Offset v);
	Operand STORM_FN xConst(Size s, Word v);

	// Relative values (dereference pointers).
	Operand STORM_FN byteRel(Register reg, Offset offset);
	Operand STORM_FN intRel(Register reg, Offset offset);
	Operand STORM_FN longRel(Register reg, Offset offset);
	Operand STORM_FN ptrRel(Register reg, Offset offset);
	Operand STORM_FN xRel(Size size, Register reg, Offset offset);

	// Access offsets inside variables.
	Operand STORM_FN byteRel(Variable v, Offset offset);
	Operand STORM_FN intRel(Variable v, Offset offset);
	Operand STORM_FN longRel(Variable v, Offset offset);
	Operand STORM_FN ptrRel(Variable v, Offset offset);
	Operand STORM_FN xRel(Size size, Variable v, Offset offset);
}
