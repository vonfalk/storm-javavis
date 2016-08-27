#include "stdafx.h"
#include "Operand.h"
#include "Exception.h"
#include "Core/StrBuf.h"

namespace code {

	static Word dual(Nat a, Nat b) {
		return Word(a) << Word(32) | Word(b);
	}

	static Nat firstDual(Word d) {
		return Nat((d >> 32) & 0xFFFFFFFF);
	}

	static Nat secondDual(Word d) {
		return Nat(d & 0xFFFFFFFF);
	}

	Operand::Operand() : opType(opNone) {}

	Operand::Operand(Register r) : opType(opRegister), opNum(r), opSize(code::size(r)) {}

	Operand::Operand(CondFlag c) : opType(opCondFlag), opNum(c), opSize(Size()) {}

	Operand::Operand(Variable v) : opType(opVariable), opNum(v.id), opSize(v.size()) {}

	Operand::Operand(Word c, Size size) : opType(opConstant), opNum(c), opSize(size) {}

	Operand::Operand(Size s, Size size) : opType(opDualConstant), opNum(dual(s.size32(), s.size64())), opSize(size) {}

	Operand::Operand(Offset o, Size size) : opType(opDualConstant), opNum(dual(o.v32(), o.v64())), opSize(size) {
		TODO(L"Check negative offsets!");
	}

	Operand::Operand(Register r, Offset offset, Size size) : opType(opRelative), opNum(r), opOffset(offset), opSize(size) {}

	Operand::Operand(Variable v, Offset offset, Size size) : opType(opVariable), opNum(v.id), opOffset(offset), opSize(size) {}

	Bool Operand::operator ==(const Operand &o) const {
		if (opType != o.opType)
			return false;

		switch (opType) {
		case opNone:
			return true;
		case opConstant:
		case opDualConstant:
		case opRegister:
		case opCondFlag:
			return opNum == o.opNum;
		case opVariable:
		case opRelative:
			return opNum == o.opNum && opOffset == o.opOffset;
		case opReference:
		default:
			assert(false, L"Unknown type!");
			return false;
		}
	}

	Bool Operand::operator !=(const Operand &o) const {
		return !(*this == o);
	}

	Bool Operand::empty() const {
		return opType == opNone;
	}

	OpType Operand::type() const {
		if (opType == opDualConstant)
			return opConstant;
		else
			return opType;
	}

	Size Operand::size() const {
		return opSize;
	}

	Bool Operand::readable() const {
		switch (opType) {
		case opNone:
		case opCondFlag:
			// TODO: Add more returning false here.
			return false;
		default:
			return true;
		}
	}

	Bool Operand::writable() const {
		switch (opType) {
		case opRegister:
		case opRelative:
		case opVariable:
			return true;
		default:
			return false;
		}
	}

	void Operand::ensureReadable(op::Code op) const {
		if (!readable())
			throw InvalidValue(L"For instruction " + String(name(op)) + L": " + ::toS(*this) + L" is not readable.");
	}

	void Operand::ensureWritable(op::Code op) const {
		if (!writable())
			throw InvalidValue(L"For instruction " + String(name(op)) + L": " + ::toS(*this) + L" is not writable.");
	}

	Word Operand::constant() const {
		assert(type() == opConstant, L"Not a constant!");
		if (opType == opConstant) {
			return opNum;
		} else if (sizeof(void *) == 4) {
			return firstDual(opNum);
		} else {
			return secondDual(opNum);
		}
	}

	Register Operand::reg() const {
		assert(type() == opRegister || type() == opRelative, L"Not a register!");
		return Register(opNum);
	}

	Offset Operand::offset() const {
		// Nothing bad happens if this is accessed wrongly.
		return opOffset;
	}

	CondFlag Operand::condFlag() const {
		assert(type() == opCondFlag, L"Not a CondFlag!");
		return CondFlag(opNum);
	}

	Variable Operand::variable() const {
		assert(type() == opVariable, L"Not a variable!");
		return Variable(Nat(opNum), opSize);
	}

	wostream &operator <<(wostream &to, const Operand &o) {
		if (o.type() != opRegister && o.type() != opCondFlag) {
			Size s = o.size();
			if (s == Size::sPtr)
				to << L"p";
			else if (s == Size::sByte)
				to << L"b";
			else if (s == Size::sInt)
				to << L"i";
			else if (s == Size::sLong)
				to << L"l";
		}

		switch (o.type()) {
		case opNone:
			return to << L"<none>";
		case opConstant:
			return to << o.constant();
		case opRegister:
			return to << code::name(o.reg());
		case opRelative:
			return to << L"[" << code::name(o.reg()) << o.offset() << L"]";
		case opVariable:
			if (o.offset() != Offset()) {
				return to << L"[Var" << o.opNum << o.offset() << L"]";
			} else {
				return to << L"[Var" << o.opNum << L"]";
			}
		case opReference:
			return to << L"TODO";
		case opCondFlag:
			return to << code::name(o.condFlag());
		default:
			assert(false, L"Unknown type!");
			return to << L"<invalid>";
		}
	}

	StrBuf &operator <<(StrBuf &to, Operand o) {
		return to << ::toS(o).c_str();
	}


	/**
	 * Create things:
	 */

	Operand byteConst(Byte v) {
		return xConst(Size::sByte, Word(v));
	}

	Operand intConst(Int v) {
		return xConst(Size::sInt, Long(v));
	}

	Operand natConst(Nat v) {
		return xConst(Size::sNat, Word(v));
	}

	Operand longConst(Long v) {
		return xConst(Size::sLong, v);
	}

	Operand wordConst(Word v) {
		return xConst(Size::sWord, v);
	}

	Operand ptrConst(Size v) {
		return Operand(v, Size::sPtr);
	}

	Operand ptrConst(Offset v) {
		return Operand(v, Size::sPtr);
	}

	Operand xConst(Size s, Word v) {
		return Operand(v, s);
	}

	Operand byteRel(Register reg, Offset offset) {
		return xRel(Size::sByte, reg, offset);
	}

	Operand intRel(Register reg, Offset offset) {
		return xRel(Size::sInt, reg, offset);
	}

	Operand longRel(Register reg, Offset offset) {
		return xRel(Size::sLong, reg, offset);
	}

	Operand ptrRel(Register reg, Offset offset) {
		return xRel(Size::sPtr, reg, offset);
	}

	Operand xRel(Size size, Register reg, Offset offset) {
		return Operand(reg, offset, Size::sPtr);
	}

	Operand byteRel(Variable v, Offset offset) {
		return xRel(Size::sByte, v, offset);
	}

	Operand intRel(Variable v, Offset offset) {
		return xRel(Size::sInt, v, offset);
	}

	Operand longRel(Variable v, Offset offset) {
		return xRel(Size::sLong, v, offset);
	}

	Operand ptrRel(Variable v, Offset offset) {
		return xRel(Size::sPtr, v, offset);
	}

	Operand xRel(Size size, Variable v, Offset offset) {
		return Operand(v, offset, size);
	}

}
