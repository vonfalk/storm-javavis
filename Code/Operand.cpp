#include "stdafx.h"
#include "Operand.h"
#include "Exception.h"
#include "Core/StrBuf.h"
#include "Core/Str.h"
#include <iomanip>

namespace code {

	// Declared here, as we do not want Storm to know about this trick!
	BITMASK_OPERATORS(OpType);

	enum OpMasks {
		opMask = 0x0FFFF,

		opRef = 0x10000,
	};

	static Word dual(Nat a, Nat b) {
		return Word(a) << Word(32) | Word(b);
	}

	static Nat firstDual(Word d) {
		return Nat((d >> 32) & 0xFFFFFFFF);
	}

	static Nat secondDual(Word d) {
		return Nat(d & 0xFFFFFFFF);
	}

	Operand::Operand() : opType(opNone), opPtr(null), opNum(0) {}

	Operand::Operand(Reg r) : opType(opRegister), opPtr(null), opNum(r), opSize(code::size(r)) {}

	Operand::Operand(CondFlag c) : opType(opCondFlag), opPtr(null), opNum(c), opSize() {}

	Operand::Operand(Part p) : opType(opPart), opPtr(null), opNum(p.id), opSize() {}

	Operand::Operand(Var v) : opType(opVariable), opPtr(null), opNum(v.id), opSize(v.size()) {}

	Operand::Operand(Label l) : opType(opLabel), opPtr(null), opNum(l.id), opSize(Size::sPtr) {}

	Operand::Operand(Ref ref) : opType(opReference), opPtr(ref.to), opNum(0), opSize(Size::sPtr) {}

	Operand::Operand(Reference *ref) : opType(opReference), opPtr(ref->to), opNum(0), opSize(Size::sPtr) {}

	Operand::Operand(Word c, Size size) : opType(opConstant), opPtr(null), opNum(c), opSize(size) {}

	Operand::Operand(Size s, Size size) : opType(opDualConstant), opPtr(null), opOffset(s), opSize(size) {}

	Operand::Operand(Offset o, Size size) : opType(opDualConstant), opPtr(null), opOffset(o), opSize(size) {}

	Operand::Operand(RootObject *obj) : opType(opObjReference), opPtr(obj), opOffset(), opSize(Size::sPtr) {}

	Operand::Operand(Reg r, Offset offset, Size size) : opType(opRelative), opPtr(null), opNum(r), opOffset(offset), opSize(size) {}

	Operand::Operand(Var v, Offset offset, Size size) : opType(opVariable), opPtr(null), opNum(v.id), opOffset(offset), opSize(size) {}

	Bool Operand::operator ==(const Operand &o) const {
		if (opType != o.opType)
			return false;

		// This is always safe to do, as 'refSize' returns Size() if no relevant value exists.
		if (refSize() != o.refSize())
			return false;

		switch (opType & opMask) {
		case opNone:
			return true;
		case opConstant:
		case opRegister:
		case opCondFlag:
		case opPart:
		case opLabel:
			return opNum == o.opNum;
		case opDualConstant:
			return opOffset == o.opOffset;
		case opVariable:
		case opRelative:
			return opNum == o.opNum && opOffset == o.opOffset;
		case opReference:
		case opObjReference:
			return opPtr == o.opPtr;
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

	Bool Operand::any() const {
		return !empty();
	}

	OpType Operand::type() const {
		OpType r = opType & OpType(opMask);

		if (r == opDualConstant)
			return opConstant;
		else
			return r;
	}

	Size Operand::size() const {
		if (opType & opRef)
			return Size::sPtr;
		else
			return opSize;
	}

	Operand Operand::referTo(Size size) const {
		if (this->size() != Size::sPtr)
			throw InvalidValue(L"Can not refer to something using a non-pointer!");
		Operand o(*this);
		o.opType = opType | OpType(opRef);
		o.opSize = size;
		return o;
	}

	Size Operand::refSize() const {
		if (opType & opRef)
			return opSize;
		else
			return Size();
	}

	Bool Operand::readable() const {
		switch (type()) {
		case opNone:
		case opCondFlag:
		case opPart:
			// TODO: Add more returning false here.
			return false;
		default:
			return true;
		}
	}

	Bool Operand::writable() const {
		switch (type()) {
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
		if ((opType & opMask) == opConstant) {
			return opNum;
		} else {
			return opOffset.current();
		}
	}

	Reg Operand::reg() const {
		assert(type() == opRegister || type() == opRelative, L"Not a register!");
		return Reg(opNum);
	}

	Offset Operand::offset() const {
		if (type() == opDualConstant)
			return Offset();

		// Nothing bad happens if this is accessed wrongly.
		return opOffset;
	}

	CondFlag Operand::condFlag() const {
		assert(type() == opCondFlag, L"Not a CondFlag!");
		return CondFlag(opNum);
	}

	Part Operand::part() const {
		assert(type() == opPart, L"Not a part!");
		return Part(Nat(opNum));
	}

	Var Operand::var() const {
		assert(type() == opVariable, L"Not a variable!");
		return Var(Nat(opNum), size());
	}

	Ref Operand::ref() const {
		assert(type() == opReference, L"Not a reference!");
		RefSource *s = (RefSource *)opPtr;
		return Ref(s);
	}

	RootObject *Operand::object() const {
		assert(type() == opObjReference, L"Not an object reference!");
		return (RootObject *)opPtr;
	}

	Label Operand::label() const {
		assert(type() == opLabel, L"Not a label!");
		return Label(Nat(opNum));
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
			if (o.size() == Size::sPtr)
				return to << L"0x" << toHex(o.constant());
			else
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
		case opLabel:
			return to << L"Label" << o.opNum;
		case opPart:
			return to << L"Part" << o.opNum;
		case opReference:
			return to << L"@" << o.ref().title();
		case opObjReference:
			TODO(L"Make sure to call on the correct OS thread!");
			return to << L"&" << o.object()->toS()->c_str();
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

	void Operand::dbg_dump() const {
		const nat *raw = (const nat *)this;
		for (nat i = 0; i < sizeof(Operand)/4; i++) {
			std::wcout << std::setw(2) << i << L": " << toHex(raw[i]) << endl;
		}
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

	Operand floatConst(Float v) {
		Nat *q = reinterpret_cast<Nat *>(&v);
		return xConst(Size::sFloat, *q);
	}

	Operand ptrConst(Size v) {
		return Operand(v, Size::sPtr);
	}

	Operand ptrConst(Offset v) {
		return Operand(v, Size::sPtr);
	}

	Operand ptrConst(Nat v) {
		return xConst(Size::sPtr, v);
	}

	Operand xConst(Size s, Word v) {
		return Operand(v, s);
	}

	Operand objPtr(Object *o) {
		return Operand(o);
	}

	Operand objPtr(TObject *o) {
		return Operand(o);
	}

	Operand byteRel(Reg reg, Offset offset) {
		return xRel(Size::sByte, reg, offset);
	}

	Operand intRel(Reg reg, Offset offset) {
		return xRel(Size::sInt, reg, offset);
	}

	Operand longRel(Reg reg, Offset offset) {
		return xRel(Size::sLong, reg, offset);
	}

	Operand ptrRel(Reg reg, Offset offset) {
		return xRel(Size::sPtr, reg, offset);
	}

	Operand xRel(Size size, Reg reg, Offset offset) {
		return Operand(reg, offset, size);
	}

	Operand byteRel(Var v, Offset offset) {
		return xRel(Size::sByte, v, offset);
	}

	Operand intRel(Var v, Offset offset) {
		return xRel(Size::sInt, v, offset);
	}

	Operand longRel(Var v, Offset offset) {
		return xRel(Size::sLong, v, offset);
	}

	Operand ptrRel(Var v, Offset offset) {
		return xRel(Size::sPtr, v, offset);
	}

	Operand xRel(Size size, Var v, Offset offset) {
		return Operand(v, offset, size);
	}

}
