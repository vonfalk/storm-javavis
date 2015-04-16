#include "StdAfx.h"
#include "Value.h"
#include "Exception.h"

namespace code {

	Value::Value() : valType(tNone), valSize(0) {}

	Value::Value(Register r) : valType(tRegister), iRegister(r), valSize(code::size(r)), iOffset(0) {
		assert(r != noReg);
	}

	Value::Value(Word v, Size sz) : valType(tConstant), iConstant(v), valSize(sz), iOffset(0) {}
	Value::Value(Size v, Size sz) : valType(tSizeConstant), iSize(v), valSize(sz), iOffset(0) {}
	Value::Value(Offset v, Size sz) : valType(tOffsetConstant), iOffset(v), valSize(sz) {}

	Value::Value(Label lbl) : valType(tLabel), labelId(lbl.id), valSize(Size::sPtr), iOffset(0) {}

	Value::Value(const Ref &ref) : valType(tReference), iReference(ref), valSize(Size::sPtr), iOffset(0) {}
	Value::Value(const RefSource &ref) : valType(tReference), iReference(ref), valSize(Size::sPtr), iOffset(0) {}

	// The size of a block value is zero, since it is an abstract entity.
	Value::Value(Part p) : valType(tPart), partId(p.id), valSize(0), iOffset(0) {}

	Value::Value(Variable v) : valType(tVariable), variableId(v.id), valSize(v.size()), iOffset(0) {
		assert(v != Variable::invalid);
	}

	// Yet another abstract entity.
	Value::Value(CondFlag f) : valType(tCondFlag), cFlag(f), valSize(0), iOffset(0) {}

	Value::Value(Register reg, Offset offset, Size sz) : valType(tRelative), valSize(sz) {
		assert(code::size(reg) == Size::sPtr);
		iRegister = reg;
		iOffset = offset;
	}

	Value::Value(Variable v, Offset offset, Size size) : valType(tVariable), valSize(size) {
		assert(v != Variable::invalid);
		assert(offset <= Offset(v.size()));
		assert(offset >= Offset());
		variableId = v.id;
		iOffset = offset;
	}

	bool Value::empty() const {
		return valType == tNone;
	}

	Value::Type Value::type() const {
		if (valType == tSizeConstant)
			return tConstant;
		if (valType == tOffsetConstant)
			return tConstant;
		return valType;
	}

	Size Value::size() const {
		return valSize;
	}

	nat Value::currentSize() const {
		if (empty())
			return 0;

		return valSize.current();
	}

	bool Value::readable() const {
		return valType != tNone && valType != tPart;
	}

	bool Value::writable() const {
		switch (valType) {
			case tRegister:
				return true;
			case tVariable:
				return true;
			case tRelative:
				return true;
		}
		return false;
	}

	bool Value::operator ==(const Value &o) const {
		if (valType != o.valType)
			return false;

		if (valSize != o.valSize)
			return false;

		switch (valType) {
		case tNone:
			return true;
		case tConstant:
			return iConstant == o.iConstant;
		case tSizeConstant:
			return iSize == o.iSize;
		case tOffsetConstant:
			return iOffset == o.iOffset;
		case tRegister:
			return iRegister == o.iRegister;
		case tLabel:
			return labelId == o.labelId;
		case tReference:
			return iReference == o.iReference;
		case tPart:
			return partId == o.partId;
		case tVariable:
			return variableId == o.variableId && iOffset == o.iOffset;
		case tRelative:
			return iRegister == o.iRegister && iOffset == o.iOffset;
		default:
			assert(false);
			return false;
		}
	}

	void Value::ensureReadable(const wchar_t *instruction) const {
		if (!readable())
			throw InvalidValue(L"For instruction " + String(instruction) + L": " + ::toS(*this) + L" is not readable");
	}

	void Value::ensureWritable(const wchar_t *instruction) const {
		if (!writable())
			throw InvalidValue(L"For instruction " + String(instruction) + L": " + ::toS(*this) + L" is not writable");
	}

	static Long maskConstant(Long val, Size size) {
		switch (size.current()) {
			case 8:
				return val;
			case 4:
				return val & 0xFFFFFFFF;
			case 1:
				return val & 0xFF;
			default:
				assert(false);
				return val;
		}
	}

	void Value::output(std::wostream &to) const {
		if (valType == tNone) {
			to << L"-";
			return;
		}

		if (valType != tRegister && valType != tCondFlag) {
			if (valSize == Size())
				to << "";
			else if (valSize == Size::sPtr)
				to << 'p';
			else if (valSize == Size::sByte)
				to << 'b';
			else if (valSize == Size::sInt)
				to << 'i';
			else if (valSize == Size::sLong)
				to << 'l';
			else
				to << valSize << ':';
		}

		switch (valType) {
		case tRegister:
			to << name(iRegister);
			break;
		case tConstant:
			to << L"#" << toHex(maskConstant(iConstant, valSize), true) << " (" << ::toS((Long)iConstant) << ")";
			break;
		case tSizeConstant:
			to << L"#" << iSize;
			break;
		case tOffsetConstant:
			to << L"#" << iOffset.format(true);
			break;
		case tLabel:
			to << L"#" << label().toS() << L":";
			break;
		case tReference:
			to << L"@" << iReference.targetName();
			to << L" (" << toHex(iReference.address(), true) << L")";
			break;
		case tPart:
			if (partId == Part::invalid.getId())
				to << L"<invalid part>";
			else
				to << L"Part" << partId;
			break;
		case tVariable:
			if (iOffset != Offset())
				to << L"[var" << variableId << iOffset.format(true) << L"]";
			else
				to << L"var" << variableId;
			break;
		case tCondFlag:
			to << name(cFlag);
			break;
		case tRelative:
			to << L"[" << name(iRegister) << iOffset.format(true) << L"]";
			break;
		default:
			assert(false);
			break;
		}

	}

	Word Value::constant() const {
		if (valType == tConstant)
			return iConstant;
		if (valType == tSizeConstant)
			return iSize.current();
		if (valType == tOffsetConstant)
			return iOffset.current();
		assert(false, "Tried to get constant value from non-constant.");
		return 0;
	}

	Register Value::reg() const {
		assert(valType == tRegister || valType == tRelative);
		return iRegister;
	}

	Label Value::label() const {
		assert(valType == tLabel);
		return Label(labelId);
	}

	Ref Value::reference() const {
		assert(valType == tReference);
		return iReference;
	}

	Part Value::part() const {
		assert(valType == tPart);
		return Part(partId);
	}

	CondFlag Value::condFlag() const {
		assert(valType == tCondFlag);
		return cFlag;
	}

	Variable Value::variable() const {
		assert(valType == tVariable);
		return Variable(variableId, valSize);
	}

	Offset Value::offset() const {
		assert(valType == tVariable || valType == tRelative || valType == tOffsetConstant);
		return iOffset;
	}

	// Creators...
	Value charConst(Char v) { return Value(Word(v), Size::sChar); }
	Value byteConst(Byte v) { return Value(Word(v), Size::sByte); }
	Value intConst(Int v) { return Value(Word(v), Size::sInt); }
	Value natConst(Nat v) { return Value(Word(v), Size::sNat); }
	Value natConst(Size v) { return Value(v, Size::sNat); }
	Value longConst(Long v) { return Value(Word(v), Size::sLong); }
	Value wordConst(Word v) { return Value(Word(v), Size::sWord); }
	Value intPtrConst(Int v) { return Value(Word(v), Size::sPtr); }
	Value natPtrConst(Nat v) { return Value(Word(v), Size::sPtr); }
	Value intPtrConst(Offset v) { return Value(v, Size::sPtr); }
	Value natPtrConst(Size v) { return Value(v, Size::sPtr); }
	Value ptrConst(Size v) { return Value(v, Size::sPtr); }
	Value ptrConst(void *v) { return Value(Word(v), Size::sPtr); }

	Value byteRel(Register reg, Offset offset) { return Value(reg, offset, Size::sByte); }
	Value intRel(Register reg, Offset offset) { return Value(reg, offset, Size::sInt); }
	Value longRel(Register reg, Offset offset) { return Value(reg, offset, Size::sLong); }
	Value ptrRel(Register reg, Offset offset) { return Value(reg, offset, Size::sPtr); }
	Value xRel(Size size, Register reg, Offset offset) { return Value(reg, offset, size); }

	Value byteRel(Variable v, Offset offset) { return Value(v, offset, Size::sByte); }
	Value intRel(Variable v, Offset offset) { return Value(v, offset, Size::sInt); }
	Value longRel(Variable v, Offset offset) { return Value(v, offset, Size::sLong); }
	Value ptrRel(Variable v, Offset offset) { return Value(v, offset, Size::sPtr); }
	Value xRel(Size size, Variable v, Offset offset) { return Value(v, offset, size); }
}
