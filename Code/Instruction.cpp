#include "StdAfx.h"
#include "Instruction.h"
#include "Arena.h"
#include "MachineCode.h"
#include "Exception.h"

using namespace code::machine; // For the OP-codes.

namespace code {

	static Value sizedReg(Register base, Size size) {
		Register s = asSize(base, size);
		if (s == noReg) {
			if (size == Size())
				return Value();
			else
				throw InvalidValue(L"The return size must fit in a register (ie < 8 bytes).");
		} else {
			return Value(s);
		}
	}

	static Instruction create(OpCode opCode) {
		return Instruction(opCode, Value(), destNone, Value());
	}

	static Instruction createSrc(OpCode opCode, const Value &src) {
		src.ensureReadable(name(opCode));
		return Instruction(opCode, Value(), destNone, src);
	}

	static Instruction createDest(OpCode opCode, const Value &dest, DestMode mode) {
		if (mode & destRead)
			dest.ensureReadable(name(opCode));
		if (mode & destWrite)
			dest.ensureReadable(name(opCode));
		return Instruction(opCode, dest, mode, Value());
	}

	static Instruction createDestSrc(OpCode opCode, const Value &dest, DestMode mode, const Value &src) {
		assert(mode != destNone);

		if (mode & destRead)
			dest.ensureReadable(name(opCode));
		if (mode & destWrite)
			dest.ensureReadable(name(opCode));

		src.ensureReadable(name(opCode));
		if (dest.size() != src.size()) {
			throw InvalidValue(String(L"For ") + name(opCode) + L": Size of operands must match! " + toS(dest) + L", " + toS(src));
		}

		return Instruction(opCode, dest, mode, src);
	}

	static Instruction createLoose(OpCode opCode, const Value &dest, DestMode destMode, const Value &src) {
		return Instruction(opCode, dest, destMode, src);
	}


	Instruction Instruction::altered(const Value &dest, const Value &src) const {
		Instruction i(*this);
		i.mySrc = src;
		i.myDest = dest;
		return i;
	}

	Instruction Instruction::alterSrc(const Value &v) const {
		Instruction i(*this);
		i.mySrc = v;
		return i;
	}

	Instruction Instruction::alterDest(const Value &v) const {
		Instruction i(*this);
		i.myDest = v;
		return i;
	}

	Instruction::Instruction() {}

	Instruction::Instruction(OpCode opCode, const Value &dest, DestMode destMode, const Value &src)
		: opCode(opCode), myDest(dest), myDestMode(destMode), mySrc(src) {}

	void Instruction::output(std::wostream &to) const {
		to << name(opCode);

		if (!myDest.empty() && !mySrc.empty())
			to << L" " << myDest << L", " << mySrc;
		else if (!myDest.empty())
			to << L" " << myDest;
		else if (!mySrc.empty())
			to << L" " << mySrc;
	}

	nat Instruction::currentSize() const {
		return max(mySrc.currentSize(), myDest.currentSize());
	}

	Size Instruction::size() const {
		return max(mySrc.size(), myDest.size());
	}

	//////////////////////////////////////////////////////////////////////////
	// OP-codes
	//////////////////////////////////////////////////////////////////////////

	Instruction mov(const Value &to, const Value &from) {
		return createDestSrc(op::mov, to, destWrite, from);
	}

	Instruction lea(const Value &to, const Value &from) {
		if (to.size() != Size::sPtr)
			throw InvalidValue(L"Lea must update a pointer.");
		switch (from.type()) {
		case Value::tRelative:
		case Value::tVariable:
		case Value::tReference:
			// These are ok.
			break;
		default:
			throw InvalidValue(L"lea must be used with a complex addressing mode or a reference."
							L" (Got " + toS(from) + L")");
		}
		return createLoose(op::lea, to, destWrite, from);
	}

	Instruction push(const Value &v) {
		return createSrc(op::push, v);
	}

	Instruction pop(const Value &to) {
		return createDest(op::pop, to, destWrite);
	}

	Instruction jmp(const Value &to, CondFlag cond) {
		if (to.size() != Size::sPtr)
			throw InvalidValue(L"Must jump to a pointer, trying to jump to " + toS(to));
		return createLoose(op::jmp, to, destRead, cond);
	}

	Instruction setCond(const Value &to, CondFlag cond) {
		if (to.size() != Size::sByte)
			throw InvalidValue(L"Must set a byte.");
		return createLoose(op::setCond, to, destWrite, cond);
	}

	Instruction call(const Value &to, Size returnSize) {
		if (to.size() != Size::sPtr)
			throw InvalidValue(L"Must call a pointer.");

		return createLoose(op::call, sizedReg(ptrA, returnSize), destWrite, to);
	}

	Instruction ret(Size returnSize) {
		Value r = sizedReg(ptrA, returnSize);
		if (r.type() == Value::tNone)
			return create(op::ret);
		else
			return createSrc(op::ret, r);
	}

	Instruction fnParam(const Value &src) {
		return createSrc(op::fnParam, src);
	}

	Instruction fnParam(const Variable &src, const Value &copyFn) {
		if (copyFn.type() != Value::tNone) {
			if (copyFn.type() == Value::tConstant)
				throw InvalidValue(L"Should not call constant values, use references instead!");
			if (copyFn.size() != Size::sPtr)
				throw InvalidValue(L"Must call a pointer.");
		}
		return createLoose(op::fnParam, copyFn, destRead, src);
	}

	Instruction fnCall(const Value &src, Size returnSize) {
		if (src.type() == Value::tConstant)
			throw InvalidValue(L"Should not call constant values, use references instead!");
		if (src.size() != Size::sPtr)
			throw InvalidValue(L"Must call a pointer.");

		return createLoose(op::fnCall, sizedReg(ptrA, returnSize), destWrite, src);
	}

	Instruction add(const Value &dest, const Value &src) {
		return createDestSrc(op::add, dest, destRead | destWrite, src);
	}

	Instruction adc(const Value &dest, const Value &src) {
		return createDestSrc(op::adc, dest, destRead | destWrite, src);
	}

	Instruction or(const Value &dest, const Value &src) {
		return createDestSrc(op::or, dest, destRead | destWrite, src);
	}

	Instruction and(const Value &dest, const Value &src) {
		return createDestSrc(op::and, dest, destRead | destWrite, src);
	}

	Instruction sub(const Value &dest, const Value &src) {
		return createDestSrc(op::sub, dest, destRead | destWrite, src);
	}

	Instruction sbb(const Value &dest, const Value &src) {
		return createDestSrc(op::sbb, dest, destRead | destWrite, src);
	}

	Instruction xor(const Value &dest, const Value &src) {
		return createDestSrc(op::xor, dest, destRead | destWrite, src);
	}

	Instruction cmp(const Value &dest, const Value &src) {
		return createDestSrc(op::cmp, dest, destRead, src);
	}

	Instruction mul(const Value &dest, const Value &src) {
		return createDestSrc(op::mul, dest, destRead | destWrite, src);
	}

	Instruction shl(const Value &dest, const Value &src) {
		if (src.size() != Size::sByte)
			throw InvalidValue(L"Size must be 1");
		return createLoose(op::shl, dest, destRead | destWrite, src);
	}

	Instruction shr(const Value &dest, const Value &src) {
		if (src.size() != Size::sByte)
			throw InvalidValue(L"Size must be 1");
		return createLoose(op::shr, dest, destRead | destWrite, src);
	}

	Instruction sar(const Value &dest, const Value &src) {
		if (src.size() != Size::sByte)
			throw InvalidValue(L"Size must be 1");
		return createLoose(op::sar, dest, destRead | destWrite, src);
	}

	Instruction fstp(const Value &dest) {
		if (dest.type() == Value::tRegister)
			throw InvalidValue(L"Can not store to register.");
		if (dest.size() != Size::sFloat)
			throw InvalidValue(L"Invalid size.");
		return createDest(op::fstp, dest, destWrite);
	}

	Instruction fistp(const Value &dest) {
		if (dest.size() != Size::sInt)
			throw InvalidValue(L"Invalid size.");
		return createDest(op::fistp, dest, destWrite);
	}

	Instruction fld(const Value &src) {
		if (src.type() == Value::tRegister)
			throw InvalidValue(L"Can not load from register.");
		if (src.type() == Value::tConstant)
			throw InvalidValue(L"Can not load from a constant.");
		if (src.size() != Size::sFloat)
			throw InvalidValue(L"Invalid size.");
		return createSrc(op::fld, src);
	}

	Instruction fild(const Value &src) {
		if (src.type() == Value::tRegister)
			throw InvalidValue(L"Can not load from register.");
		if (src.type() == Value::tConstant)
			throw InvalidValue(L"Can not load from a constant.");
		if (src.size() != Size::sInt)
			throw InvalidValue(L"Invalid size.");
		return createSrc(op::fild, src);
	}

	Instruction faddp() {
		return create(op::faddp);
	}

	Instruction fsubp() {
		return create(op::fsubp);
	}

	Instruction fmulp() {
		return create(op::fmulp);
	}

	Instruction fdivp() {
		return create(op::fdivp);
	}

	Instruction fwait() {
		return create(op::fwait);
	}

	Instruction retFloat(const Size &s) {
		Value r = sizedReg(ptrA, s);
		return createSrc(op::retFloat, r);
	}


	Instruction dat(const Value &v) {
		switch (v.type()) {
			case Value::tConstant:
			case Value::tLabel:
			case Value::tReference:
				break;
			default:
				throw InvalidValue(L"Cannot store other than references, constants and labels in dat");
		}
		return createSrc(op::dat, v);
	}

	Instruction addRef(const Value &to) {
		return createSrc(op::addRef, to);
	}

	Instruction releaseRef(const Value &to) {
		return createSrc(op::releaseRef, to);
	}

	Instruction prolog() {
		return create(op::prolog);
	}

	Instruction epilog() {
		return create(op::epilog);
	}

	Instruction begin(Part block) {
		return createLoose(op::beginBlock, Value(), destNone, block);
	}

	Instruction end(Part block) {
		return createLoose(op::endBlock, Value(), destNone, Part(block));
	}

	Instruction threadLocal() {
		return create(op::threadLocal);
	}
}
