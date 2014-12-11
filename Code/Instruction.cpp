#include "StdAfx.h"
#include "Instruction.h"
#include "Arena.h"
#include "MachineCode.h"
#include "Errors.h"

using namespace code::machine; // For the OP-codes.

namespace code {

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
		if (dest.sizeType() != src.sizeType()) {
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

	Instruction::Instruction(OpCode opCode, const Value &dest, DestMode destMode, const Value &src) : opCode(opCode), myDest(dest), myDestMode(destMode), mySrc(src) {}

	void Instruction::output(std::wostream &to) const {
		to << name(opCode);

		switch (opCode) {
		case op::jmp:
		case op::setCond:
			to << " " << name(CondFlag(mySrc.constant())) << L" " << myDest;
			return;
		}

		if (!myDest.empty() && !mySrc.empty())
			to << L" " << myDest << L", " << mySrc;
		else if (!myDest.empty())
			to << L" " << myDest;
		else if (!mySrc.empty())
			to << L" " << mySrc;
	}

	nat Instruction::size() const {
		return max(mySrc.size(), myDest.size());
	}

	// CondFlag.
	const wchar_t *name(CondFlag c) {
		switch (c) {
		case ifAlways:
			return L"always";
		case ifNever:
			return L"never";
		case ifOverflow:
			return L"overflow";
		case ifNoOverflow:
			return L"no overflow";
		case ifEqual:
			return L"equal";
		case ifNotEqual:
			return L"not equal";
		case ifBelow:
			return L"below";
		case ifBelowEqual:
			return L"below/equal";
		case ifAboveEqual:
			return L"above/equal";
		case ifAbove:
			return L"above";
		case ifLess:
			return L"less";
		case ifLessEqual:
			return L"less/equal";
		case ifGreaterEqual:
			return L"greater/equal";
		case ifGreater:
			return L"greater";
		}

		TODO(L"Implement!");
		assert(false);
		return L"Unknown CondFlag";
	}

	CondFlag inverse(CondFlag c) {
		switch (c) {
		case ifAlways:
			return ifNever;
		case ifNever:
			return ifAlways;
		case ifOverflow:
			return ifNoOverflow;
		case ifNoOverflow:
			return ifOverflow;
		case ifEqual:
			return ifNotEqual;
		case ifNotEqual:
			return ifEqual;
		case ifBelow:
			return ifAboveEqual;
		case ifBelowEqual:
			return ifAbove;
		case ifAboveEqual:
			return ifBelow;
		case ifAbove:
			return ifBelowEqual;
		case ifLess:
			return ifGreaterEqual;
		case ifLessEqual:
			return ifGreater;
		case ifGreaterEqual:
			return ifLess;
		case ifGreater:
			return ifLessEqual;
		}

		TODO(L"Implement!");
		assert(false);
		return ifNever;
	}

	//////////////////////////////////////////////////////////////////////////
	// OP-codes
	//////////////////////////////////////////////////////////////////////////

	Instruction mov(const Value &to, const Value &from) {
		return createDestSrc(op::mov, to, destWrite, from);
	}

	Instruction push(const Value &v) {
		return createSrc(op::push, v);
	}

	Instruction pop(const Value &to) {
		return createDest(op::pop, to, destWrite);
	}

	Instruction jmp(const Value &to, CondFlag cond) {
		if (to.sizeType() != 0)
			throw InvalidValue(L"Must jump to a pointer.");
		return createLoose(op::jmp, to, destRead, intConst(cond));
	}

	Instruction setCond(const Value &to, CondFlag cond) {
		if (to.size() != 1)
			throw InvalidValue(L"Must set a byte.");
		return createLoose(op::setCond, to, destWrite, intConst(cond));
	}

	Instruction call(const Value &to, nat returnSize) {
		if (to.sizeType() != 0)
			throw InvalidValue(L"Must call a pointer.");
		if (returnSize > 8)
			throw InvalidValue(L"Size must be below or equal to 8.");
		return createLoose(op::call, asSize(ptrA, returnSize), destWrite, to);
	}

	Instruction ret(nat returnSize) {
		if (returnSize > 8)
			throw InvalidValue(L"Size must be below or equal to 8.");
		return createSrc(op::ret, asSize(ptrA, returnSize));
	}

	Instruction fnParam(const Value &src) {
		return createSrc(op::fnParam, src);
	}

	Instruction fnCall(const Value &src, nat returnSize) {
		if (src.type() == Value::tConstant)
			throw InvalidValue(L"Should not call constant values, use references instead!");
		if (src.sizeType() != 0)
			throw InvalidValue(L"Must call a pointer.");
		if (returnSize > 8)
			throw InvalidValue(L"Size must be below or equal to 8.");
		return createLoose(op::fnCall, asSize(ptrA, returnSize), destWrite, src);
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
		if (src.size() != 1)
			throw InvalidValue(L"Size must be 1");
		return createLoose(op::shl, dest, destRead | destWrite, src);
	}

	Instruction shr(const Value &dest, const Value &src) {
		if (src.size() != 1)
			throw InvalidValue(L"Size must be 1");
		return createLoose(op::shr, dest, destRead | destWrite, src);
	}

	Instruction sar(const Value &dest, const Value &src) {
		if (src.size() != 1)
			throw InvalidValue(L"Size must be 1");
		return createLoose(op::sar, dest, destRead | destWrite, src);
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

	Instruction begin(Block block) {
		return createLoose(op::beginBlock, Value(), destNone, block);
	}

	Instruction end(Block block) {
		return createLoose(op::endBlock, Value(), destNone, block);
	}

	Instruction threadLocal() {
		return create(op::threadLocal);
	}
}
