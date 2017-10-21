#pragma once
#include "Reg.h"
#include "OpCode.h"
#include "CondFlag.h"
#include "Var.h"
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

		// Relative to a label. TODO: Implement for X86!
		opRelativeLbl,

		// Variable (+ offset).
		opVariable,

		// Label.
		opLabel,

		// Block or part.
		opPart,

		// Reference to another object.
		opReference,

		// Reference to an object.
		opObjReference,

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
		STORM_CAST_CTOR Operand(Reg reg);

		// CondFlag.
		STORM_CAST_CTOR Operand(CondFlag flag);

		// Variable.
		STORM_CAST_CTOR Operand(Var var);

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
		Bool STORM_FN any() const;

		// Type.
		OpType STORM_FN type() const;

		// Size.
		Size STORM_FN size() const;

		// Make this operand a reference to some value of a specific size. Only works if this
		// operand is already pointer-sized.
		// TODO: Remove this!
		Operand STORM_FN referTo(Size size) const;

		// Get the size of the value we're referring to (if any), or Size() if we're not referring to anything.
		Size STORM_FN refSize() const;

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
		Reg STORM_FN reg() const; // "register" is a reserved word.
		Offset STORM_FN offset() const;
		CondFlag STORM_FN condFlag() const;
		Part STORM_FN part() const;
		Label STORM_FN label() const;
		Ref STORM_FN ref() const;
		RefSource *refSource() const;
		RootObject *object() const;
		Var STORM_FN var() const; // NOTE: The size of this variable is equal to the size
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
		Operand(RootObject *obj);

		// [Register,variable,label] + offset.
		Operand(Reg r, Offset offset, Size size);
		Operand(Var v, Offset offset, Size size);
		Operand(Label l, Offset offset, Size size);

		// Our type. Note: may contain other flags as well!
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
		friend Operand objPtr(Object *ptr);
		friend Operand objPtr(TObject *ptr);
		friend Operand xRel(Size size, Reg reg, Offset offset);
		friend Operand xRel(Size size, Var v, Offset offset);
		friend Operand xRel(Size size, Label l, Offset offset);
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
	Operand STORM_FN doubleConst(Double v);
	Operand STORM_FN ptrConst(Size v);
	Operand STORM_FN ptrConst(Offset v);
	Operand STORM_FN ptrConst(Nat v);
	Operand STORM_FN xConst(Size s, Word v);

	// Store a pointer to a GC:d object as a constant.
	Operand STORM_FN objPtr(Object *ptr);
	Operand STORM_FN objPtr(TObject *ptr);

	// Relative values (dereference pointers).
	Operand STORM_FN byteRel(Reg reg, Offset offset);
	Operand STORM_FN intRel(Reg reg, Offset offset);
	Operand STORM_FN longRel(Reg reg, Offset offset);
	Operand STORM_FN floatRel(Reg reg, Offset offset);
	Operand STORM_FN doubleRel(Reg reg, Offset offset);
	Operand STORM_FN ptrRel(Reg reg, Offset offset);
	Operand STORM_FN xRel(Size size, Reg reg, Offset offset);

	// Access offsets inside variables.
	Operand STORM_FN byteRel(Var v, Offset offset);
	Operand STORM_FN intRel(Var v, Offset offset);
	Operand STORM_FN longRel(Var v, Offset offset);
	Operand STORM_FN floatRel(Var v, Offset offset);
	Operand STORM_FN doubleRel(Var v, Offset offset);
	Operand STORM_FN ptrRel(Var v, Offset offset);
	Operand STORM_FN xRel(Size size, Var v, Offset offset);

	// Access offsets inside variables.
	Operand STORM_FN byteRel(Label l, Offset offset);
	Operand STORM_FN intRel(Label l, Offset offset);
	Operand STORM_FN longRel(Label l, Offset offset);
	Operand STORM_FN floatRel(Label l, Offset offset);
	Operand STORM_FN doubleRel(Label l, Offset offset);
	Operand STORM_FN ptrRel(Label l, Offset offset);
	Operand STORM_FN xRel(Size size, Label l, Offset offset);
}
