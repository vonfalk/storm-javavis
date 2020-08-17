#pragma once
#include "Reg.h"
#include "OpCode.h"
#include "CondFlag.h"
#include "Var.h"
#include "Block.h"
#include "Label.h"
#include "Reference.h"
#include "Core/SrcPos.h"

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
		STORM_NAME(opNone, none),

		// Constant.
		STORM_NAME(opConstant, constant),

		// Dual-constant (one value for 32-bit and one for 64-bit). Appears as 'opConstant', but is stored in 'opOffset'.
		STORM_NAME(opDualConstant, dualConstant),

		// Register
		STORM_NAME(opRegister, register),

		// Relative to a register. ie. [reg + offset]
		STORM_NAME(opRelative, relative),

		// Relative to a register, the offset represented by a reference.
		STORM_NAME(opRelativeRef, relativeRef),

		// Relative to a label. TODO: Implement for X86!
		STORM_NAME(opRelativeLbl, relativeLabel),

		// Variable (+ offset).
		STORM_NAME(opVariable, variable),

		// Variable (+ offset, originating from a reference).
		STORM_NAME(opVariableRef, variableRef),

		// Label.
		STORM_NAME(opLabel, label),

		// Block.
		STORM_NAME(opBlock, block),

		// Reference to another object.
		STORM_NAME(opReference, reference),

		// Reference to an object.
		STORM_NAME(opObjReference, objReference),

		// Condition flag.
		STORM_NAME(opCondFlag, condFlag),

		// Source code reference (SrcPos).
		STORM_NAME(opSrcPos, srcPos),
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
		STORM_CAST_CTOR Operand(Block part);

		// Label.
		STORM_CAST_CTOR Operand(Label label);

		// Reference.
		STORM_CAST_CTOR Operand(Ref ref);
		STORM_CAST_CTOR Operand(Reference *ref);

		// SrcPos.
		STORM_CAST_CTOR Operand(SrcPos pos);

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

		// Is this value readable and/or writable?
		Bool STORM_FN readable() const;
		Bool STORM_FN writable() const;

		// Throw an exception unless the value is readable/writable.
		void STORM_FN ensureReadable(op::OpCode op) const;
		void STORM_FN ensureWritable(op::OpCode op) const;

		// Does this operand use a register (Note: local variables that will use ptrBase later are
		// not considered to be using a register). This currently includes raw registers and memory
		// accesses relative to registers.
		Bool STORM_FN hasRegister() const;

		/**
		 * Get values. Asserts if you try to get a value we're not representing.
		 */

		// Constant (depends on the backend).
		Word STORM_FN constant() const;
		Reg STORM_FN reg() const; // "register" is a reserved word.
		Offset STORM_FN offset() const;
		CondFlag STORM_FN condFlag() const;
		Block STORM_FN block() const;
		Label STORM_FN label() const;
		Ref STORM_FN ref() const;
		RefSource *refSource() const;
		RootObject *object() const;
		Var STORM_FN var() const; // NOTE: The size of this variable is equal to the size
		                          // we want to read, which is not always the size of the
								  // original variable!

		SrcPos STORM_FN srcPos() const;

		MAYBE(Object *) STORM_FN obj() const;
		MAYBE(TObject *) STORM_FN tObj() const;

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

		// [register,variable,label] + offset.
		Operand(Reg r, Offset offset, Size size);
		Operand(Var v, Offset offset, Size size);
		Operand(Label l, Offset offset, Size size);

		// [register,variable] + offset + ref
		Operand(Reg r, Offset offset, Ref ref, Size size);
		Operand(Var r, Offset offset, Ref ref, Size size);

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
		friend Operand intConst(Offset v);
		friend Operand natConst(Size v);
		friend Operand ptrConst(Size v);
		friend Operand ptrConst(Offset v);
		friend Operand xConst(Size s, Word w);
		friend Operand objPtr(Object *ptr);
		friend Operand objPtr(TObject *ptr);
		friend Operand xRel(Size size, Reg reg, Offset offset);
		friend Operand xRel(Size size, Var v, Offset offset);
		friend Operand xRel(Size size, Reg reg, Ref ref, Offset offset);
		friend Operand xRel(Size size, Var v, Ref ref, Offset offset);
		friend Operand xRel(Size size, Label l, Offset offset);
		friend wostream &operator <<(wostream &to, const Operand &o);
		friend StrBuf &operator <<(StrBuf &to, Operand o);
	};

	// Output.
	wostream &operator <<(wostream &to, const Operand &o);
	StrBuf &STORM_FN operator <<(StrBuf &to, Operand o);

	// Create constants. Note: Do not try to create a pointer to a Gc:d object using xConst. That
	// will fail miserably, as the pointer will not be updated whenever the Gc moves the object.
	Operand STORM_FN byteConst(Byte v);
	Operand STORM_FN intConst(Int v);
	Operand STORM_FN intConst(Offset v);
	Operand STORM_FN natConst(Nat v);
	Operand STORM_FN natConst(Size v);
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
	inline Operand STORM_FN xRel(Size size, Reg reg, Offset offset) { return Operand(reg, offset, size); }
	inline Operand STORM_FN byteRel(Reg reg, Offset offset) { return xRel(Size::sByte, reg, offset); }
	inline Operand STORM_FN intRel(Reg reg, Offset offset) { return xRel(Size::sInt, reg, offset); }
	inline Operand STORM_FN longRel(Reg reg, Offset offset) { return xRel(Size::sLong, reg, offset); }
	inline Operand STORM_FN floatRel(Reg reg, Offset offset) { return xRel(Size::sFloat, reg, offset); }
	inline Operand STORM_FN doubleRel(Reg reg, Offset offset) { return xRel(Size::sDouble, reg, offset); }
	inline Operand STORM_FN ptrRel(Reg reg, Offset offset) { return xRel(Size::sPtr, reg, offset); }

	inline Operand STORM_FN xRel(Size size, Reg reg) { return xRel(size, reg, Offset()); }
	inline Operand STORM_FN byteRel(Reg reg) { return xRel(Size::sByte, reg, Offset()); }
	inline Operand STORM_FN intRel(Reg reg) { return xRel(Size::sInt, reg, Offset()); }
	inline Operand STORM_FN longRel(Reg reg) { return xRel(Size::sLong, reg, Offset()); }
	inline Operand STORM_FN floatRel(Reg reg) { return xRel(Size::sFloat, reg, Offset()); }
	inline Operand STORM_FN doubleRel(Reg reg) { return xRel(Size::sDouble, reg, Offset()); }
	inline Operand STORM_FN ptrRel(Reg reg) { return xRel(Size::sPtr, reg, Offset()); }

	inline Operand STORM_FN xRel(Size size, Reg reg, Ref ref, Offset offset) { return Operand(reg, offset, ref, size); }
	inline Operand STORM_FN byteRel(Reg reg, Ref ref, Offset offset) { return xRel(Size::sByte, reg, ref, offset); }
	inline Operand STORM_FN intRel(Reg reg, Ref ref, Offset offset) { return xRel(Size::sInt, reg, ref, offset); }
	inline Operand STORM_FN longRel(Reg reg, Ref ref, Offset offset) { return xRel(Size::sLong, reg, ref, offset); }
	inline Operand STORM_FN floatRel(Reg reg, Ref ref, Offset offset) { return xRel(Size::sFloat, reg, ref, offset); }
	inline Operand STORM_FN doubleRel(Reg reg, Ref ref, Offset offset) { return xRel(Size::sDouble, reg, ref, offset); }
	inline Operand STORM_FN ptrRel(Reg reg, Ref ref, Offset offset) { return xRel(Size::sPtr, reg, ref, offset); }

	inline Operand STORM_FN xRel(Size size, Reg reg, Ref ref) { return xRel(size, reg, ref, Offset()); }
	inline Operand STORM_FN byteRel(Reg reg, Ref ref) { return byteRel(reg, ref, Offset()); }
	inline Operand STORM_FN intRel(Reg reg, Ref ref) { return intRel(reg, ref, Offset()); }
	inline Operand STORM_FN longRel(Reg reg, Ref ref) { return longRel(reg, ref, Offset()); }
	inline Operand STORM_FN floatRel(Reg reg, Ref ref) { return floatRel(reg, ref, Offset()); }
	inline Operand STORM_FN doubleRel(Reg reg, Ref ref) { return doubleRel(reg, ref, Offset()); }
	inline Operand STORM_FN ptrRel(Reg reg, Ref ref) { return ptrRel(reg, ref, Offset()); }

	// Access offsets inside variables.
	inline Operand STORM_FN xRel(Size size, Var v, Offset offset) { return Operand(v, offset, size); }
	inline Operand STORM_FN byteRel(Var v, Offset offset) { return xRel(Size::sByte, v, offset); }
	inline Operand STORM_FN intRel(Var v, Offset offset) { return xRel(Size::sInt, v, offset); }
	inline Operand STORM_FN longRel(Var v, Offset offset) { return xRel(Size::sLong, v, offset); }
	inline Operand STORM_FN floatRel(Var v, Offset offset) { return xRel(Size::sFloat, v, offset); }
	inline Operand STORM_FN doubleRel(Var v, Offset offset) { return xRel(Size::sDouble, v, offset); }
	inline Operand STORM_FN ptrRel(Var v, Offset offset) { return xRel(Size::sPtr, v, offset); }

	inline Operand STORM_FN xRel(Size size, Var v) { return xRel(size, v, Offset()); }
	inline Operand STORM_FN byteRel(Var v) { return xRel(Size::sByte, v, Offset()); }
	inline Operand STORM_FN intRel(Var v) { return xRel(Size::sInt, v, Offset()); }
	inline Operand STORM_FN longRel(Var v) { return xRel(Size::sLong, v, Offset()); }
	inline Operand STORM_FN floatRel(Var v) { return xRel(Size::sFloat, v, Offset()); }
	inline Operand STORM_FN doubleRel(Var v) { return xRel(Size::sDouble, v, Offset()); }
	inline Operand STORM_FN ptrRel(Var v) { return xRel(Size::sPtr, v, Offset()); }

	inline Operand STORM_FN xRel(Size size, Var v, Ref ref, Offset offset) { return Operand(v, offset, ref, size); }
	inline Operand STORM_FN byteRel(Var v, Ref ref, Offset offset) { return xRel(Size::sByte, v, ref, offset); }
	inline Operand STORM_FN intRel(Var v, Ref ref, Offset offset) { return xRel(Size::sInt, v, ref, offset); }
	inline Operand STORM_FN longRel(Var v, Ref ref, Offset offset) { return xRel(Size::sLong, v, ref, offset); }
	inline Operand STORM_FN floatRel(Var v, Ref ref, Offset offset) { return xRel(Size::sFloat, v, ref, offset); }
	inline Operand STORM_FN doubleRel(Var v, Ref ref, Offset offset) { return xRel(Size::sDouble, v, ref, offset); }
	inline Operand STORM_FN ptrRel(Var v, Ref ref, Offset offset) { return xRel(Size::sPtr, v, ref, offset); }

	inline Operand STORM_FN xRel(Size size, Var v, Ref ref) { return xRel(size, v, ref, Offset()); }
	inline Operand STORM_FN byteRel(Var v, Ref ref) { return byteRel(v, ref, Offset()); }
	inline Operand STORM_FN intRel(Var v, Ref ref) { return intRel(v, ref, Offset()); }
	inline Operand STORM_FN longRel(Var v, Ref ref) { return longRel(v, ref, Offset()); }
	inline Operand STORM_FN floatRel(Var v, Ref ref) { return floatRel(v, ref, Offset()); }
	inline Operand STORM_FN doubleRel(Var v, Ref ref) { return doubleRel(v, ref, Offset()); }
	inline Operand STORM_FN ptrRel(Var v, Ref ref) { return ptrRel(v, ref, Offset()); }

	// Access offsets inside labels.
	inline Operand STORM_FN xRel(Size size, Label l, Offset offset) { return Operand(l, offset, size); }
	inline Operand STORM_FN byteRel(Label l, Offset offset) { return xRel(Size::sByte, l, offset); }
	inline Operand STORM_FN intRel(Label l, Offset offset) { return xRel(Size::sInt, l, offset); }
	inline Operand STORM_FN longRel(Label l, Offset offset) { return xRel(Size::sLong, l, offset); }
	inline Operand STORM_FN floatRel(Label l, Offset offset) { return xRel(Size::sFloat, l, offset); }
	inline Operand STORM_FN doubleRel(Label l, Offset offset) { return xRel(Size::sDouble, l, offset); }
	inline Operand STORM_FN ptrRel(Label l, Offset offset) { return xRel(Size::sPtr, l, offset); }

	inline Operand STORM_FN xRel(Size size, Label l) { return xRel(size, l, Offset()); }
	inline Operand STORM_FN byteRel(Label l) { return xRel(Size::sByte, l, Offset()); }
	inline Operand STORM_FN intRel(Label l) { return xRel(Size::sInt, l, Offset()); }
	inline Operand STORM_FN longRel(Label l) { return xRel(Size::sLong, l, Offset()); }
	inline Operand STORM_FN floatRel(Label l) { return xRel(Size::sFloat, l, Offset()); }
	inline Operand STORM_FN doubleRel(Label l) { return xRel(Size::sDouble, l, Offset()); }
	inline Operand STORM_FN ptrRel(Label l) { return xRel(Size::sPtr, l, Offset()); }
}
