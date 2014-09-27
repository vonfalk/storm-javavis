#pragma once

#include "Register.h"
#include "Label.h"
#include "Reference.h"
#include "Block.h"

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
			// block()
			tBlock,
			// variable(), offset() (read at [&variable() + offset()])
			tVariable,
			// reg(), offset() (read at [reg() + offset()])
			tRelative,
		};

		Value();
		Value(Register r);
		Value(Label lbl);
		Value(const Ref &ref);
		Value(Block block);
		Value(Variable variable);

		bool operator ==(const Value &o) const;
		inline bool operator !=(const Value &o) const { return !(*this == o); }

		bool empty() const;

		inline Type type() const { return valType; }

		bool writable() const;
		bool readable() const;

		// Get the size of this value (1, 4, or 8 bytes). Pointer sizes depend on the backend.
		nat size() const;

		// Returns the same as "size", except for pointers, which returns 0.
		nat sizeType() const;

		// These throw an appropriate exception if the value is not X.
		void ensureWritable(const wchar_t *instruction) const;
		void ensureReadable(const wchar_t *instruction) const;

		Word constant() const;
		Register reg() const; // "register" is a reserved word...
		Label label() const;
		Ref reference() const;
		Block block() const;
		Variable variable() const;
		int offset() const;

	private:
		// Use the creators below for these:

		// Constant
		Value(Word c, nat size);

		// Reference
		Value(Register r, int offset, nat size);

		// Variable + offset
		Value(Variable v, int offset, nat size);

		// Type
		Type valType;

		// Type description. (size == 0 => pointer)
		nat valSize;

		union {
			// tConstant:
			Word iConstant;
			// tRegister:
			Register iRegister;
			// label id
			nat labelId;
			// block id
			nat blockId;
			// variable id
			nat variableId;
		};

		// Offset relative either a register or a variable.
		int iOffset;

		// Only valid if tReference, otherwise a null reference.
		Ref iReference;

		void output(std::wostream &to) const;

		// Friend declarations.
		friend Value charConst(Char v);
		friend Value byteConst(Byte v);
		friend Value intConst(Int v);
		friend Value natConst(Nat v);
		friend Value longConst(Long v);
		friend Value wordConst(Word v);
		friend Value intPtrConst(Int v);
		friend Value natPtrConst(Nat v);
		friend Value byteRel(Register reg, int offset);
		friend Value intRel(Register reg, int offset);
		friend Value longRel(Register reg, int offset);
		friend Value ptrRel(Register reg, int offset);
		friend Value byteRel(Variable v, int offset);
		friend Value intRel(Variable v, int offset);
		friend Value longRel(Variable v, int offset);
		friend Value ptrRel(Variable v, int offset);
	};

	// Create constants.
	Value charConst(Char v);
	Value byteConst(Byte v);
	Value intConst(Int v);
	Value natConst(Nat v);
	Value longConst(Long v);
	Value wordConst(Word v);

	// size_t like constants. Cannot be more than 32-bits for compatibility.
	Value intPtrConst(Int v);
	Value natPtrConst(Nat v);

	// Create relative values.
	Value byteRel(Register reg, int offset);
	Value intRel(Register reg, int offset);
	Value longRel(Register reg, int offset);
	Value ptrRel(Register reg, int offset);

	// Create values relative to variable locations.
	Value byteRel(Variable v, int offset);
	Value intRel(Variable v, int offset);
	Value longRel(Variable v, int offset);
	Value ptrRel(Variable v, int offset);
}
