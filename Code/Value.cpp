#include "StdAfx.h"
#include "Value.h"
#include "Errors.h"

namespace code {

	Value::Value() : valType(tNone), valSize(0) {}

	Value::Value(Register r) : valType(tRegister), iRegister(r), valSize(code::size(r)), iOffset(0) {
		assert(r != noReg);
	}

	Value::Value(Word v, nat sz) : valType(tConstant), iConstant(v), valSize(sz), iOffset(0) {}

	Value::Value(Label lbl) : valType(tLabel), labelId(lbl.id), valSize(0), iOffset(0) {}

	Value::Value(const Ref &ref) : valType(tReference), iReference(ref), valSize(0), iOffset(0) {}

	Value::Value(Block b) : valType(tBlock), blockId(b.id), valSize(0), iOffset(0) {}

	Value::Value(Variable v) : valType(tVariable), blockId(v.id), valSize(v.size()), iOffset(0) {
		assert(v != Variable::invalid);
	}

	Value::Value(Register reg, cpuInt offset, nat sz) : valType(tRelative), valSize(sz) {
		assert(code::size(reg) == 0);
		iRegister = reg;
		iOffset = offset;
	}

	Value::Value(Variable v, int offset, nat size) : valType(tVariable), valSize(size) {
		assert(v != Variable::invalid);
		assert(offset + size <= v.size());
		assert(offset >= 0);
		variableId = v.id;
		iOffset = offset;
	}

	bool Value::empty() const {
		return valType == tNone;
	}

	nat Value::sizeType() const {
		return valSize;
	}

	nat Value::size() const {
		if (empty())
			return 0;

		if (valSize == 0)
			return sizeof(cpuNat);
		else
			return valSize;
	}

	bool Value::readable() const {
		return valType != tNone && valType != tBlock;
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
			case tRegister:
				return iRegister == o.iRegister;
			case tLabel:
				return labelId == o.labelId;
			case tReference:
				return iReference == o.iReference;
			case tBlock:
				return blockId == o.blockId;
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
			throw InvalidValue(L"For instruction " + String(instruction) + L": " + toS(*this) + L" is not readable");
	}

	void Value::ensureWritable(const wchar_t *instruction) const {
		if (!writable())
			throw InvalidValue(L"For instruction " + String(instruction) + L": " + toS(*this) + L" is not writable");
	}

	static Long maskConstant(Long val, nat size) {
		if (size == 0)
			size = sizeof(cpuNat);

		switch (size) {
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

		if (valType != tRegister) {
			const wchar_t types[] = L"pb??i???l";
			to << types[valSize];
		}

		switch (valType) {
			case tRegister:
				to << name(iRegister);
				break;
			case tConstant:
				to << L"#" << toHex(maskConstant(iConstant, valSize), true) << " (" << toS((Long)iConstant) << ")";
				break;
			case tLabel:
				to << L"#" << label().toString() << L":";
				break;
			case tReference:
				to << L"@" << iReference.targetName();
				break;
			case tBlock:
				to << L"Block" << blockId;
				break;
			case tVariable:
				if (iOffset != 0)
					to << L"[var" << variableId << (iOffset >= 0 ? L"+" : L"-") << toHex(abs(iOffset), true) << L"]";
				else
					to << L"var" << variableId;
				break;
			case tRelative:
				to << L"[" << name(iRegister) << (iOffset >= 0 ? L"+" : L"-") << toHex(abs(iOffset), true) << L"]";
				break;
			default:
				assert(false);
				break;
		}

	}

	Word Value::constant() const {
		assert(valType == tConstant);
		return iConstant;
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

	Block Value::block() const {
		assert(valType == tBlock);
		return Block(blockId);
	}

	Variable Value::variable() const {
		assert(valType == tVariable);
		return Variable(variableId, valSize);
	}

	int Value::offset() const {
		assert(valType == tVariable || valType == tRelative);
		return iOffset;
	}

	// Creators...
	Value charConst(Char v) { return Value(Word(v), 1); }
	Value byteConst(Byte v) { return Value(Word(v), 1); }
	Value intConst(Int v) { return Value(Word(v), 4); }
	Value natConst(Nat v) { return Value(Word(v), 4); }
	Value longConst(Long v) { return Value(Word(v), 8); }
	Value wordConst(Word v) { return Value(Word(v), 8); }
	Value intPtrConst(Int v) { return Value(Word(v), 0); }
	Value natPtrConst(Nat v) { return Value(Word(v), 0); }


	Value byteRel(Register reg, int offset) { return Value(reg, offset, 1); }
	Value intRel(Register reg, int offset) { return Value(reg, offset, 4); }
	Value longRel(Register reg, int offset) { return Value(reg, offset, 8); }
	Value ptrRel(Register reg, int offset) { return Value(reg, offset, 0); }

	Value byteRel(Variable v, int offset) { return Value(v, offset, 1); }
	Value intRel(Variable v, int offset) { return Value(v, offset, 4); }
	Value longRel(Variable v, int offset) { return Value(v, offset, 8); }
	Value ptrRel(Variable v, int offset) { return Value(v, offset, 0); }
}