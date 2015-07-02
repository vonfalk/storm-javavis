#pragma once

#include "Register.h"
#include "Label.h"
#include "Reference.h"
#include "Block.h"
#include "CondFlag.h"

namespace code {

	// Class describing a value in an ASM listing. A value may be one of:
	// A constant
	// A register
	// A pointer
	// A pointer relative a register
	// A label
	// A reference
	// A variable
	// A block
	class Value : public Printable {
	public:

		// Value type
		enum Type {
			// No type
			tNone,
			// constant()
			tConstant,
			// reg()
			tRegister,
			// label()
			tLabel,
			// reference()
			tReference,
			// part()
			tPart,
			// variable(), offset() (read at [&variable() + offset()])
			tVariable,
			// condFlag()
			tCondFlag,
			// reg(), offset() (read at [reg() + offset()])
			tRelative,

			// Internal, appears as a constant. Stored in iSize/iOffset.
			tSizeConstant, tOffsetConstant, tFloatConstant,
		};

		Value();
		Value(Register r);
		Value(Label lbl);
		Value(const Ref &ref);
		Value(const RefSource &ref);
		Value(Part part);
		Value(Variable variable);
		Value(CondFlag condFlag);

		bool operator ==(const Value &o) const;
		inline bool operator !=(const Value &o) const { return !(*this == o); }

		bool empty() const;

		Type type() const;

		bool writable() const;
		bool readable() const;

		// Get the size of this value.
		Size size() const;

		// Get the size of this value (1, 4, or 8 bytes). Pointer sizes depend on the backend.
		nat currentSize() const;

		// These throw an appropriate exception if the value is not X.
		void ensureWritable(const wchar_t *instruction) const;
		void ensureReadable(const wchar_t *instruction) const;

		Word constant() const; // if the constant is a size, the size on this platform is returned.
		Register reg() const; // "register" is a reserved word...
		Label label() const;
		Ref reference() const;
		Part part() const;
		Variable variable() const;
		CondFlag condFlag() const;
		Offset offset() const;

	private:
		// Use the creators below for these:

		// Constant
		Value(Word c, Size size);
		Value(Size s, Size size);
		Value(Offset s, Size size);
		Value(Float c, Size size);

		// Reference
		Value(Register r, Offset offset, Size size);

		// Variable + offset
		Value(Variable v, Offset offset, Size size);

		// Type
		Type valType;

		// Type description.
		Size valSize;

		union {
			// tConstant:
			Word iConstant;
			// float constant.
			Float fConstant;
			// tRegister:
			Register iRegister;
			// label id
			nat labelId;
			// part id
			nat partId;
			// variable id
			nat variableId;
			// conditional flag
			CondFlag cFlag;
		};

		// Offset relative either a register or a variable.
		Offset iOffset;

		// Size.
		Size iSize;

		// Only valid if tReference, otherwise a null reference.
		Ref iReference;

		void output(std::wostream &to) const;

		// Friend declarations.
		friend Value charConst(Char v);
		friend Value byteConst(Byte v);
		friend Value intConst(Int v);
		friend Value natConst(Nat v);
		friend Value natConst(Size v);
		friend Value longConst(Long v);
		friend Value wordConst(Word v);
		friend Value floatConst(Float v);
		friend Value intPtrConst(Int v);
		friend Value natPtrConst(Nat v);
		friend Value intPtrConst(Offset v);
		friend Value natPtrConst(Size v);
		friend Value ptrConst(Size v);
		friend Value ptrConst(void *v);
		friend Value byteRel(Register reg, Offset offset);
		friend Value intRel(Register reg, Offset offset);
		friend Value longRel(Register reg, Offset offset);
		friend Value floatRel(Register reg, Offset offset);
		friend Value ptrRel(Register reg, Offset offset);
		friend Value xRel(Size size, Register reg, Offset offset);
		friend Value byteRel(Variable v, Offset offset);
		friend Value intRel(Variable v, Offset offset);
		friend Value longRel(Variable v, Offset offset);
		friend Value floatRel(Variable v, Offset offset);
		friend Value ptrRel(Variable v, Offset offset);
		friend Value xRel(Size size, Variable v, Offset offset);
	};

	// Create constants.
	Value charConst(Char v);
	Value byteConst(Byte v);
	Value intConst(Int v);
	Value natConst(Nat v);
	Value natConst(Size v);
	Value longConst(Long v);
	Value wordConst(Word v);
	Value floatConst(Float v);

	// size_t like constants. Cannot be more than 32-bits for compatibility.
	Value intPtrConst(Int v);
	Value natPtrConst(Nat v);
	Value intPtrConst(Offset v);
	Value natPtrConst(Size v);
	Value ptrConst(Size v);
	Value ptrConst(void *v); // careful with this, will break miserably if serialized and loaded.

	// Create relative values.
	Value byteRel(Register reg, Offset offset = Offset(0));
	Value intRel(Register reg, Offset offset = Offset(0));
	Value longRel(Register reg, Offset offset = Offset(0));
	Value floatRel(Register reg, Offset offset = Offset(0));
	Value ptrRel(Register reg, Offset offset = Offset(0));
	Value xRel(Size size, Register reg, Offset offset = Offset(0));

	// Create values relative to variable locations (relative to the beginning of the variable, not its content).
	Value byteRel(Variable v, Offset offset = Offset(0));
	Value intRel(Variable v, Offset offset = Offset(0));
	Value longRel(Variable v, Offset offset = Offset(0));
	Value floatRel(Variable v, Offset offset = Offset(0));
	Value ptrRel(Variable v, Offset offset = Offset(0));
	Value xRel(Size size, Variable v, Offset offset = Offset(0));

}
