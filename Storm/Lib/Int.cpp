#include "stdafx.h"
#include "Int.h"
#include "Engine.h"
#include "Code.h"
#include "Function.h"
#include "CodeGen.h"
#include "Number.h"
#include "Shared/Hash.h"

namespace storm {
	using namespace code;

	static void intAssign(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << mov(intRel(ptrA), p.params[1]);
		if (p.result->needed()) {
			if (p.result->type().ref) {
				if (!p.result->suggest(p.state, p.params[0]))
					p.state->to << mov(p.result->location(p.state).var(), ptrA);
			} else {
				if (!p.result->suggest(p.state, p.params[1]))
					p.state->to << mov(p.result->location(p.state).var(), p.params[1]);
			}
		}
	}

	static void intCopyCtor(InlinedParams p) {
		p.state->to << mov(ptrC, p.params[1]);
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << mov(intRel(ptrA), intRel(ptrC));
	}


	static void intToFloat(InlinedParams p) {
		if (!p.result->needed())
			return;
		p.state->to << fild(p.params[0]);
		p.state->to << fstp(p.result->location(p.state).var());
	}

	IntType::IntType() : Type(L"Int", typeValue | typeFinal, Size::sInt, null) {}

	bool IntType::loadAll() {
		vector<Value> r(1, Value(this, true));
		vector<Value> v(1, Value(this));
		vector<Value> vv(2, Value(this));
		vector<Value> rv = valList(2, Value(this, true), Value(this));
		vector<Value> rr(2, Value(this, true));
		Value b(boolType(engine));

		add(steal(inlinedFunction(engine, Value(this), L"+", vv, simpleFn(&numAdd))));
		add(steal(inlinedFunction(engine, Value(this), L"-", vv, simpleFn(&numSub))));
		add(steal(inlinedFunction(engine, Value(this), L"*", vv, simpleFn(&numMul))));
		add(steal(inlinedFunction(engine, Value(this), L"/", vv, simpleFn(&numIdiv))));
		add(steal(inlinedFunction(engine, Value(this), L"%", vv, simpleFn(&numImod))));
		add(steal(inlinedFunction(engine, b, L"==", vv, simpleFn(&numCmp<ifEqual>))));
		add(steal(inlinedFunction(engine, b, L"!=", vv, simpleFn(&numCmp<ifNotEqual>))));
		add(steal(inlinedFunction(engine, b, L"<", vv, simpleFn(&numCmp<ifLess>))));
		add(steal(inlinedFunction(engine, b, L">", vv, simpleFn(&numCmp<ifGreater>))));
		add(steal(inlinedFunction(engine, b, L"<=", vv, simpleFn(&numCmp<ifLessEqual>))));
		add(steal(inlinedFunction(engine, b, L">=", vv, simpleFn(&numCmp<ifGreaterEqual>))));

		add(steal(inlinedFunction(engine, Value(natType(engine)), L"nat", v, simpleFn(&icast))));
		add(steal(inlinedFunction(engine, Value(byteType(engine)), L"byte", v, simpleFn(&icast))));
		add(steal(inlinedFunction(engine, Value(longType(engine)), L"long", v, simpleFn(&icast))));
		add(steal(inlinedFunction(engine, Value(wordType(engine)), L"word", v, simpleFn(&icast))));
		add(steal(inlinedFunction(engine, Value(floatType(engine)), L"float", v, simpleFn(&intToFloat))));

		add(steal(inlinedFunction(engine, Value(this), L"*++", r, simpleFn(&numPostfixInc<Int>))));
		add(steal(inlinedFunction(engine, Value(this), L"++*", r, simpleFn(&numPrefixInc<Int>))));
		add(steal(inlinedFunction(engine, Value(this), L"*--", r, simpleFn(&numPostfixDec<Int>))));
		add(steal(inlinedFunction(engine, Value(this), L"--*", r, simpleFn(&numPrefixDec<Int>))));

		add(steal(inlinedFunction(engine, Value(this, true), L"=", rv, simpleFn(&intAssign))));
		add(steal(inlinedFunction(engine, Value(this), L"+=", rv, simpleFn(&numInc<Int>))));
		add(steal(inlinedFunction(engine, Value(this), L"-=", rv, simpleFn(&numDec<Int>))));
		add(steal(inlinedFunction(engine, Value(this), L"*=", rv, simpleFn(&numScale<Int>))));
		add(steal(inlinedFunction(engine, Value(this), L"/=", rv, simpleFn(&numIDivScale<Int>))));
		add(steal(inlinedFunction(engine, Value(this), L"%=", rv, simpleFn(&numIModEq<Int>))));

		add(steal(inlinedFunction(engine, Value(), Type::CTOR, rr, simpleFn(&intCopyCtor))));

		add(steal(nativeFunction(engine, Value(natType(engine)), L"hash", v, &intHash)));

		return Type::loadAll();
	}

	Type *intType(Engine &e) {
		Type *t = e.specialBuiltIn(specialInt);
		if (!t) {
			t = CREATE(IntType, e);
			e.setSpecialBuiltIn(specialInt, t);
		}
		return t;
	}

	static void natAssign(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << mov(intRel(ptrA), p.params[1]);
		if (p.result->needed()) {
			if (p.result->type().ref) {
				if (!p.result->suggest(p.state, p.params[0]))
					p.state->to << mov(p.result->location(p.state).var(), ptrA);
			} else {
				if (!p.result->suggest(p.state, p.params[1]))
					p.state->to << mov(p.result->location(p.state).var(), p.params[1]);
			}
		}
	}

	static void natCopyCtor(InlinedParams p) {
		p.state->to << mov(ptrC, p.params[1]);
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << mov(intRel(ptrA), intRel(ptrC));
	}


	static void createNat(InlinedParams p) {
		p.state->to << mov(ptrC, p.params[0]);
		p.state->to << ucast(intRel(ptrC), p.params[1]);
	}

	NatType::NatType() : Type(L"Nat", typeValue | typeFinal, Size::sNat, null) {}

	bool NatType::loadAll() {
		vector<Value> r(1, Value(this, true));
		vector<Value> v(1, Value(this));
		vector<Value> vv(2, Value(this));
		vector<Value> rv = valList(2, Value(this, true), Value(this));
		vector<Value> rr(2, Value(this, true));
		Value b(boolType(engine));

		add(steal(inlinedFunction(engine, Value(this), L"+", vv, simpleFn(&numAdd))));
		add(steal(inlinedFunction(engine, Value(this), L"-", vv, simpleFn(&numSub))));
		add(steal(inlinedFunction(engine, Value(this), L"*", vv, simpleFn(&numMul))));
		add(steal(inlinedFunction(engine, Value(this), L"/", vv, simpleFn(&numUdiv))));
		add(steal(inlinedFunction(engine, Value(this), L"%", vv, simpleFn(&numUmod))));

		add(steal(inlinedFunction(engine, b, L"==", vv, simpleFn(&numCmp<ifEqual>))));
		add(steal(inlinedFunction(engine, b, L"!=", vv, simpleFn(&numCmp<ifNotEqual>))));
		add(steal(inlinedFunction(engine, b, L"<", vv, simpleFn(&numCmp<ifBelow>))));
		add(steal(inlinedFunction(engine, b, L">", vv, simpleFn(&numCmp<ifAbove>))));
		add(steal(inlinedFunction(engine, b, L"<=", vv, simpleFn(&numCmp<ifBelowEqual>))));
		add(steal(inlinedFunction(engine, b, L">=", vv, simpleFn(&numCmp<ifAboveEqual>))));

		add(steal(inlinedFunction(engine, Value(intType(engine)), L"int", v, simpleFn(&ucast))));
		add(steal(inlinedFunction(engine, Value(wordType(engine)), L"word", v, simpleFn(&ucast))));
		add(steal(inlinedFunction(engine, Value(longType(engine)), L"long", v, simpleFn(&ucast))));
		add(steal(inlinedFunction(engine, Value(byteType(engine)), L"byte", v, simpleFn(&ucast))));

		add(steal(inlinedFunction(engine, Value(this), L"*++", r, simpleFn(&numPostfixInc<Nat>))));
		add(steal(inlinedFunction(engine, Value(this), L"++*", r, simpleFn(&numPrefixInc<Nat>))));
		add(steal(inlinedFunction(engine, Value(this), L"*--", r, simpleFn(&numPostfixDec<Nat>))));
		add(steal(inlinedFunction(engine, Value(this), L"--*", r, simpleFn(&numPrefixDec<Nat>))));

		vector<Value> rn = valList(2, Value(this, true), Value(this));
		add(steal(inlinedFunction(engine, Value(this, true), L"=", rn, simpleFn(&natAssign))));
		add(steal(inlinedFunction(engine, Value(this), L"+=", rn, simpleFn(&numInc<Nat>))));
		add(steal(inlinedFunction(engine, Value(this), L"-=", rn, simpleFn(&numDec<Nat>))));
		add(steal(inlinedFunction(engine, Value(this), L"*=", rv, simpleFn(&numScale<Nat>))));
		add(steal(inlinedFunction(engine, Value(this), L"/=", rv, simpleFn(&numUDivScale<Nat>))));
		add(steal(inlinedFunction(engine, Value(this), L"%=", rv, simpleFn(&numUModEq<Nat>))));

		add(steal(inlinedFunction(engine, Value(), Type::CTOR, rr, simpleFn(&natCopyCtor))));

		vector<Value> rb = valList(2, Value(this, true), Value(byteType(engine)));
		add(stealAutoCast(inlinedFunction(engine, Value(), Type::CTOR, rb, simpleFn(&createNat))));

		add(steal(nativeFunction(engine, Value(this), L"hash", v, &natHash)));

		return Type::loadAll();
	}


	Type *natType(Engine &e) {
		Type *t = e.specialBuiltIn(specialNat);
		if (!t) {
			t = CREATE(NatType, e);
			e.setSpecialBuiltIn(specialNat, t);
		}
		return t;
	}

	static void byteAssign(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << mov(byteRel(ptrA), p.params[1]);
		if (p.result->needed()) {
			if (p.result->type().ref) {
				if (!p.result->suggest(p.state, p.params[0]))
					p.state->to << mov(p.result->location(p.state).var(), ptrA);
			} else {
				if (!p.result->suggest(p.state, p.params[1]))
					p.state->to << mov(p.result->location(p.state).var(), p.params[1]);
			}
		}
	}

	static void byteCopyCtor(InlinedParams p) {
		p.state->to << mov(ptrC, p.params[1]);
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << mov(byteRel(ptrA), byteRel(ptrC));
	}

	ByteType::ByteType() : Type(L"Byte", typeValue | typeFinal, Size::sByte, null) {}

	bool ByteType::loadAll() {
		vector<Value> r(1, Value(this, true));
		vector<Value> v(1, Value(this));
		vector<Value> vv(2, Value(this));
		vector<Value> rv = valList(2, Value(this, true), Value(this));
		vector<Value> rr(2, Value(this, true));
		Value b(boolType(engine));

		add(steal(inlinedFunction(engine, Value(this), L"+", vv, simpleFn(&numAdd))));
		add(steal(inlinedFunction(engine, Value(this), L"-", vv, simpleFn(&numSub))));
		// These are not supported yet.
		// add(steal(inlinedFunction(engine, Value(this), L"*", vv, simpleFn(&numMul))));
		// add(steal(inlinedFunction(engine, Value(this), L"/", vv, simpleFn(&numUdiv))));
		// add(steal(inlinedFunction(engine, Value(this), L"%", vv, simpleFn(&numUmod))));

		add(steal(inlinedFunction(engine, b, L"==", vv, simpleFn(&numCmp<ifEqual>))));
		add(steal(inlinedFunction(engine, b, L"!=", vv, simpleFn(&numCmp<ifNotEqual>))));
		add(steal(inlinedFunction(engine, b, L"<", vv, simpleFn(&numCmp<ifBelow>))));
		add(steal(inlinedFunction(engine, b, L">", vv, simpleFn(&numCmp<ifAbove>))));
		add(steal(inlinedFunction(engine, b, L"<=", vv, simpleFn(&numCmp<ifBelowEqual>))));
		add(steal(inlinedFunction(engine, b, L">=", vv, simpleFn(&numCmp<ifAboveEqual>))));

		add(steal(inlinedFunction(engine, Value(intType(engine)), L"int", v, simpleFn(&ucast))));
		add(steal(inlinedFunction(engine, Value(natType(engine)), L"nat", v, simpleFn(&ucast))));
		add(steal(inlinedFunction(engine, Value(wordType(engine)), L"word", v, simpleFn(&ucast))));
		add(steal(inlinedFunction(engine, Value(longType(engine)), L"long", v, simpleFn(&ucast))));

		add(steal(inlinedFunction(engine, Value(this), L"*++", r, simpleFn(&numPostfixInc<Byte>))));
		add(steal(inlinedFunction(engine, Value(this), L"++*", r, simpleFn(&numPrefixInc<Byte>))));
		add(steal(inlinedFunction(engine, Value(this), L"*--", r, simpleFn(&numPostfixDec<Byte>))));
		add(steal(inlinedFunction(engine, Value(this), L"--*", r, simpleFn(&numPrefixDec<Byte>))));

		add(steal(inlinedFunction(engine, Value(this, true), L"=", rv, simpleFn(&byteAssign))));
		add(steal(inlinedFunction(engine, Value(this), L"+=", rv, simpleFn(&numInc<Byte>))));
		add(steal(inlinedFunction(engine, Value(this), L"-=", rv, simpleFn(&numDec<Byte>))));

		add(steal(inlinedFunction(engine, Value(), Type::CTOR, rr, simpleFn(&byteCopyCtor))));

		add(steal(nativeFunction(engine, Value(natType(engine)), L"hash", v, &byteHash)));

		return Type::loadAll();
	}


	Type *byteType(Engine &e) {
		Type *t = e.specialBuiltIn(specialByte);
		if (!t) {
			t = CREATE(ByteType, e);
			e.setSpecialBuiltIn(specialByte, t);
		}
		return t;
	}


	Str *toS(EnginePtr e, Int v) {
		return CREATE(Str, e.v, ::toS(v));
	}

	Str *toS(EnginePtr e, Nat v) {
		return CREATE(Str, e.v, ::toS(v));
	}

	Str *toS(EnginePtr e, Byte v) {
		return CREATE(Str, e.v, ::toS(v));
	}

}
