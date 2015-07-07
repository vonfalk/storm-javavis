#include "stdafx.h"
#include "AsmWrap.h"
#include "Shared/Str.h"

namespace storm {

	/**
	 * Size.
	 */

	wrap::Size::Size() {}

	wrap::Size::Size(Nat v) : v(v) {}

	wrap::Size::Size(const code::Size &v) : v(v) {}

	Bool wrap::Size::operator ==(const Size &o) const {
		return v == o.v;
	}

	wrap::Size wrap::Size::operator +(const Size &o) const {
		return v + o.v;
	}

	wrap::Size &wrap::Size::operator +=(const Size &o) {
		v += o.v;
		return *this;
	}

	wrap::Size wrap::sizeChar() {
		return code::Size::sChar;
	}

	wrap::Size wrap::sizeByte() {
		return code::Size::sByte;
	}

	wrap::Size wrap::sizeInt() {
		return code::Size::sInt;
	}

	wrap::Size wrap::sizeNat() {
		return code::Size::sNat;
	}

	wrap::Size wrap::sizeLong() {
		return code::Size::sLong;
	}

	wrap::Size wrap::sizeWord() {
		return code::Size::sWord;
	}

	wrap::Size wrap::sizePtr() {
		return code::Size::sPtr;
	}

	wostream &wrap::operator <<(wostream &to, const wrap::Size &s) {
		return to << s.v;
	}

	Str *wrap::toS(EnginePtr e, wrap::Size s) {
		return CREATE(Str, e.v, ::toS(s));
	}

	/**
	 * Offset.
	 */

	wrap::Offset::Offset() {}

	wrap::Offset::Offset(const code::Offset &v) : v(v) {}

	Bool wrap::Offset::operator ==(const Offset &o) const {
		return v == o.v;
	}

	wrap::Offset wrap::Offset::operator +(const Offset &o) const {
		return v + o.v;
	}

	wrap::Offset &wrap::Offset::operator +=(const Offset &o) {
		v += o.v;
		return *this;
	}

	wostream &wrap::operator <<(wostream &to, const wrap::Offset &s) {
		return to << s.v;
	}

	Str *wrap::toS(EnginePtr e, wrap::Offset s) {
		return CREATE(Str, e.v, ::toS(s));
	}

	wrap::Offset wrap::offsetChar() {
		return code::Offset::sChar;
	}

	wrap::Offset wrap::offsetByte() {
		return code::Offset::sByte;
	}

	wrap::Offset wrap::offsetInt() {
		return code::Offset::sInt;
	}

	wrap::Offset wrap::offsetNat() {
		return code::Offset::sNat;
	}

	wrap::Offset wrap::offsetLong() {
		return code::Offset::sLong;
	}

	wrap::Offset wrap::offsetWord() {
		return code::Offset::sWord;
	}

	wrap::Offset wrap::offsetPtr() {
		return code::Offset::sPtr;
	}

	/**
	 * Variable
	 */

	wostream &wrap::operator <<(wostream &to, const wrap::Variable &s) {
		return to << code::Value(s.v);
	}

	Str *wrap::toS(EnginePtr e, wrap::Variable s) {
		return CREATE(Str, e.v, ::toS(s));
	}

	/**
	 * Block.
	 */

	wostream &wrap::operator <<(wostream &to, const wrap::Block &s) {
		return to << code::Value(s.v);
	}

	Str *wrap::toS(EnginePtr e, wrap::Block s) {
		return CREATE(Str, e.v, ::toS(s));
	}

	/**
	 * Part.
	 */

	wostream &wrap::operator <<(wostream &to, const wrap::Part &s) {
		return to << code::Value(s.v);
	}

	Str *wrap::toS(EnginePtr e, wrap::Part s) {
		return CREATE(Str, e.v, ::toS(s));
	}

	/**
	 * Label.
	 */

	wostream &wrap::operator <<(wostream &to, const wrap::Label &s) {
		return to << s.v.toS();
	}

	Str *wrap::toS(EnginePtr e, wrap::Label s) {
		return CREATE(Str, e.v, ::toS(s));
	}

	/**
	 * Register.
	 */

	wrap::Register::Register() : v(code::noReg) {}

	wostream &wrap::operator <<(wostream &to, const wrap::Register &s) {
		return to << s.v;
	}

	Str *wrap::toS(EnginePtr e, wrap::Register s) {
		return CREATE(Str, e.v, ::toS(s));
	}

	wrap::Register wrap::ptrStack() {
		return Register(code::ptrStack);
	}

	wrap::Register wrap::ptrFrame() {
		return Register(code::ptrFrame);
	}

	wrap::Register wrap::ptrA() {
		return Register(code::ptrA);
	}

	wrap::Register wrap::ptrB() {
		return Register(code::ptrB);
	}

	wrap::Register wrap::ptrC() {
		return Register(code::ptrC);
	}

	wrap::Register wrap::al() {
		return Register(code::al);
	}

	wrap::Register wrap::bl() {
		return Register(code::bl);
	}

	wrap::Register wrap::cl() {
		return Register(code::cl);
	}

	wrap::Register wrap::eax() {
		return Register(code::eax);
	}

	wrap::Register wrap::ebx() {
		return Register(code::ebx);
	}

	wrap::Register wrap::ecx() {
		return Register(code::ecx);
	}

	wrap::Register wrap::rax() {
		return Register(code::rax);
	}

	wrap::Register wrap::rbx() {
		return Register(code::rbx);
	}

	wrap::Register wrap::rcx() {
		return Register(code::rcx);
	}

	/**
	 * CondFlag.
	 */
	wostream &wrap::operator <<(wostream &to, const wrap::CondFlag &s) {
		return to << name(s.v);
	}

	Str *wrap::toS(EnginePtr e, wrap::CondFlag s) {
		return CREATE(Str, e.v, ::toS(s));
	}

	wrap::CondFlag wrap::CondFlag::inverse() const {
		return code::inverse(v);
	}

	wrap::CondFlag wrap::ifAlways() {
		return code::ifAlways;
	}

	wrap::CondFlag wrap::ifNever() {
		return code::ifNever;
	}

	wrap::CondFlag wrap::ifOverflow() {
		return code::ifOverflow;
	}

	wrap::CondFlag wrap::ifNoOverflow() {
		return code::ifNoOverflow;
	}

	wrap::CondFlag wrap::ifEqual() {
		return code::ifEqual;
	}

	wrap::CondFlag wrap::ifNotEqual() {
		return code::ifNotEqual;
	}

	wrap::CondFlag wrap::ifBelow() {
		return code::ifBelow;
	}

	wrap::CondFlag wrap::ifBelowEqual() {
		return code::ifBelowEqual;
	}

	wrap::CondFlag wrap::ifAboveEqual() {
		return code::ifAboveEqual;
	}

	wrap::CondFlag wrap::ifAbove() {
		return code::ifAbove;
	}

	wrap::CondFlag wrap::ifLess() {
		return code::ifLess;
	}

	wrap::CondFlag wrap::ifLessEqual() {
		return code::ifLessEqual;
	}

	wrap::CondFlag wrap::ifGreaterEqual() {
		return code::ifGreaterEqual;
	}

	wrap::CondFlag wrap::ifGreater() {
		return code::ifGreater;
	}

	/**
	 * Operand.
	 */

	wrap::Operand::Operand() {}

	wrap::Operand::Operand(Register r) : v(r.v) {}

	wrap::Operand::Operand(CondFlag flag) : v(flag.v) {}

	wrap::Operand::Operand(Variable v) : v(v.v) {}

	wrap::Operand::Operand(Part p) : v(p.v) {}

	wrap::Operand::Operand(Block b) : v(b.v) {}

	wrap::Operand::Operand(Label l) : v(l.v) {}

	wrap::Operand::Operand(const code::Value &v) : v(v) {}

	Bool wrap::Operand::operator ==(const Operand &o) const {
		return v == o.v;
	}

	wrap::Size wrap::Operand::size() const {
		return v.size();
	}

	wostream &wrap::operator <<(wostream &to, const wrap::Operand &o) {
		return to << o.v;
	}

	Str *wrap::toS(EnginePtr e, wrap::Operand o) {
		return CREATE(Str, e.v, ::toS(o));
	}

	wrap::Operand wrap::byteConst(Byte v) {
		return code::byteConst(v);
	}

	wrap::Operand wrap::intConst(Int v) {
		return code::intConst(v);
	}

	wrap::Operand wrap::natConst(Nat v) {
		return code::natConst(v);
	}

	wrap::Operand wrap::natConst(Size v) {
		return code::natConst(v.v);
	}

	wrap::Operand wrap::intPtrConst(Int v) {
		return code::intPtrConst(v);
	}

	wrap::Operand wrap::natPtrConst(Nat v) {
		return code::natPtrConst(v);
	}

	wrap::Operand wrap::ptrConst(Size v) {
		return code::ptrConst(v.v);
	}

	wrap::Operand wrap::byteRel(Register reg, Offset offset) {
		return code::byteRel(reg.v, offset.v);
	}

	wrap::Operand wrap::intRel(Register reg, Offset offset) {
		return code::intRel(reg.v, offset.v);
	}

	wrap::Operand wrap::longRel(Register reg, Offset offset) {
		return code::longRel(reg.v, offset.v);
	}

	wrap::Operand wrap::ptrRel(Register reg, Offset offset) {
		return code::ptrRel(reg.v, offset.v);
	}

	wrap::Operand wrap::xRel(Size size, Register reg, Offset offset) {
		return code::xRel(size.v, reg.v, offset.v);
	}

	wrap::Operand wrap::byteRel(Variable v, Offset offset) {
		return code::byteRel(v.v, offset.v);
	}

	wrap::Operand wrap::intRel(Variable v, Offset offset) {
		return code::intRel(v.v, offset.v);
	}

	wrap::Operand wrap::longRel(Variable v, Offset offset) {
		return code::longRel(v.v, offset.v);
	}

	wrap::Operand wrap::ptrRel(Variable v, Offset offset) {
		return code::ptrRel(v.v, offset.v);
	}

	wrap::Operand wrap::xRel(Size size, Variable v, Offset offset) {
		return code::xRel(size.v, v.v, offset.v);
	}

	/**
	 * RetVal.
	 */

	wostream &wrap::operator <<(wostream &to, const wrap::RetVal &s) {
		if (s.isFloat)
			to << L"float:";
		to << s.size;
		return to;
	}

	Str *wrap::toS(EnginePtr e, wrap::RetVal s) {
		return CREATE(Str, e.v, ::toS(s));
	}

	/**
	 * Instruction.
	 */

	wrap::Instruction::Instruction(const code::Instruction &v) : v(v) {}

	wostream &wrap::operator <<(wostream &to, const wrap::Instruction &i) {
		return to << i.v;
	}

	Str *wrap::toS(EnginePtr e, wrap::Instruction i) {
		return CREATE(Str, e.v, ::toS(i));
	}

	wrap::Instruction wrap::prolog() {
		return code::prolog();
	}

	wrap::Instruction wrap::epilog() {
		return code::epilog();
	}

	wrap::Instruction wrap::call(Operand to, RetVal r) {
		return code::call(to.v, r.v());
	}

	wrap::Instruction wrap::ret(RetVal s) {
		return code::ret(s.v());
	}

	wrap::Instruction wrap::fnCall(Operand to, RetVal ret) {
		return code::fnCall(to.v, ret.v());
	}

	wrap::Instruction wrap::fnParam(Operand param) {
		return code::fnParam(param.v);
	}

	wrap::Instruction wrap::fnParam(Variable param, Operand copy) {
		return code::fnParam(param.v, copy.v);
	}


	wrap::Instruction wrap::jmp(Operand to) {
		return code::jmp(to.v);
	}

	wrap::Instruction wrap::jmp(Operand to, CondFlag cond) {
		return code::jmp(to.v, cond.v);
	}

	wrap::Instruction wrap::mov(Operand to, Operand from) {
		return code::mov(to.v, from.v);
	}

	wrap::Instruction wrap::lea(Operand to, Operand from) {
		return code::lea(to.v, from.v);
	}

	wrap::Instruction wrap::addRef(Operand v) {
		return code::addRef(v.v);
	}

	wrap::Instruction wrap::releaseRef(Operand v) {
		return code::releaseRef(v.v);
	}

	wrap::Instruction wrap::add(Operand dest, Operand src) {
		return code::add(dest.v, src.v);
	}

	wrap::Instruction wrap::adc(Operand dest, Operand src) {
		return code::adc(dest.v, src.v);
	}

	wrap::Instruction wrap::or(Operand dest, Operand src) {
		return code::or(dest.v, src.v);
	}

	wrap::Instruction wrap::and(Operand dest, Operand src) {
		return code::and(dest.v, src.v);
	}

	wrap::Instruction wrap::sub(Operand dest, Operand src) {
		return code::sub(dest.v, src.v);
	}

	wrap::Instruction wrap::sbb(Operand dest, Operand src) {
		return code::sbb(dest.v, src.v);
	}

	wrap::Instruction wrap::xor(Operand dest, Operand src) {
		return code::xor(dest.v, src.v);
	}

	wrap::Instruction wrap::cmp(Operand dest, Operand src) {
		return code::cmp(dest.v, src.v);
	}

	wrap::Instruction wrap::mul(Operand dest, Operand src) {
		return code::mul(dest.v, src.v);
	}

	wrap::Instruction wrap::push(Operand v) {
		return code::push(v.v);
	}

	wrap::Instruction wrap::pop(Operand v) {
		return code::pop(v.v);
	}


	/**
	 * FreeOn.
	 */

	wostream &wrap::operator <<(wostream &to, const wrap::FreeOn &v) {
		return to << v.v;
	}

	Str *wrap::toS(EnginePtr e, wrap::FreeOn o) {
		return CREATE(Str, e.v, ::toS(o));
	}

	wrap::FreeOn wrap::freeOnNone() {
		return code::freeOnNone;
	}

	wrap::FreeOn wrap::freeOnException() {
		return code::freeOnException;
	}

	wrap::FreeOn wrap::freeOnBlockExit() {
		return code::freeOnBlockExit;
	}

	wrap::FreeOn wrap::freeOnBoth() {
		return code::freeOnBoth;
	}

	wrap::FreeOn wrap::freePtr() {
		return code::freePtr;
	}


	/**
	 * Listing.
	 */

	wrap::Listing::Listing() {}

	wrap::Listing::Listing(Par<Listing> o) : v(o->v) {}

	wrap::Listing *wrap::Listing::operator <<(const Instruction &v) {
		this->v << v.v;
		addRef();
		return this;
	}

	wrap::Listing *wrap::Listing::operator <<(const Label &l) {
		this->v << l.v;
		addRef();
		return this;
	}

	wrap::Label wrap::Listing::label() {
		return v.label();
	}

	void wrap::Listing::output(wostream &to) const {
		to << v;
	}

	wrap::Block wrap::Listing::root() {
		return v.frame.root();
	}

	wrap::Block wrap::Listing::createChild(Part parent) {
		return v.frame.createChild(parent.v);
	}

	wrap::Part wrap::Listing::createPart(Part after) {
		return v.frame.createPart(after.v);
	}

	void wrap::Listing::delay(Variable var, Part to) {
		return v.frame.delay(var.v, to.v);
	}


	wrap::Variable wrap::Listing::createByteVar(Part in, Operand free, FreeOn on) {
		return v.frame.createByteVar(in.v, free.v, on.v);
	}

	wrap::Variable wrap::Listing::createIntVar(Part in, Operand free, FreeOn on) {
		return v.frame.createIntVar(in.v, free.v, on.v);
	}

	wrap::Variable wrap::Listing::createLongVar(Part in, Operand free, FreeOn on) {
		return v.frame.createLongVar(in.v, free.v, on.v);
	}

	wrap::Variable wrap::Listing::createPtrVar(Part in, Operand free, FreeOn on) {
		return v.frame.createPtrVar(in.v, free.v, on.v);
	}

	wrap::Variable wrap::Listing::createVariable(Part in, Size size, Operand free, FreeOn on) {
		return v.frame.createVariable(in.v, size.v, free.v, on.v);
	}


	wrap::Variable wrap::Listing::createByteParam(Operand free, FreeOn on) {
		return v.frame.createByteParam(free.v, on.v);
	}

	wrap::Variable wrap::Listing::createIntParam(Operand free, FreeOn on) {
		return v.frame.createIntParam(free.v, on.v);
	}

	wrap::Variable wrap::Listing::createLongParam(Operand free, FreeOn on) {
		return v.frame.createLongParam(free.v, on.v);
	}

	wrap::Variable wrap::Listing::createPtrParam(Operand free, FreeOn on) {
		return v.frame.createPtrParam(free.v, on.v);
	}

	wrap::Variable wrap::Listing::createParameter(Size size, Bool isFloat, Operand free, FreeOn on) {
		return v.frame.createParameter(size.v, isFloat, free.v, on.v);
	}

}
