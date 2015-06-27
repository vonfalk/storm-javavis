#include "stdafx.h"
#include "Int.h"
#include "Engine.h"
#include "Code.h"
#include "Function.h"
#include "CodeGen.h"

namespace storm {
	using namespace code;

	static void intAdd(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << mov(result, p.params[0]);
			p.state->to << add(result, p.params[1]);
		}
	}

	static void intInc(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << add(intRel(ptrA), p.params[1]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), intRel(ptrA));
	}

	static void intDec(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << sub(intRel(ptrA), p.params[1]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), intRel(ptrA));
	}

	static void intPrefixInc(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << add(intRel(ptrA), intConst(1));
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), intRel(ptrA));
	}

	static void intPostfixInc(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), intRel(ptrA));
		p.state->to << add(intRel(ptrA), intConst(1));
	}

	static void intPrefixDec(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << sub(intRel(ptrA), intConst(1));
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), intRel(ptrA));
	}

	static void intPostfixDec(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), intRel(ptrA));
		p.state->to << sub(intRel(ptrA), intConst(1));
	}

	static void intSub(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << mov(result, p.params[0]);
			p.state->to << sub(result, p.params[1]);
		}
	}

	static void intMul(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << mov(result, p.params[0]);
			p.state->to << mul(result, p.params[1]);
		}
	}

	static void intDiv(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << mov(result, p.params[0]);
			p.state->to << idiv(result, p.params[1]);
		}
	}

	static void intMod(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << mov(result, p.params[0]);
			p.state->to << imod(result, p.params[1]);
		}
	}

	template <CondFlag f>
	static void intCmp(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << cmp(p.params[0], p.params[1]);
			p.state->to << setCond(result, f);
		}
	}

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


	static void intToNat(InlinedParams p) {
		if (!p.result->needed())
			return;
		p.state->to << mov(p.result->location(p.state).var(), p.params[0]);
	}

	static void intToByte(InlinedParams p) {
		if (!p.result->needed())
			return;
		p.state->to << lea(ptrA, p.params[0]);
		p.state->to << mov(p.result->location(p.state).var(), byteRel(ptrA));
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
		vector<Value> i(1, Value(this));
		vector<Value> ii(2, Value(this));
		Value b(boolType(engine));
		add(steal(inlinedFunction(engine, Value(this), L"+", ii, simpleFn(&intAdd))));
		add(steal(inlinedFunction(engine, Value(this), L"-", ii, simpleFn(&intSub))));
		add(steal(inlinedFunction(engine, Value(this), L"*", ii, simpleFn(&intMul))));
		add(steal(inlinedFunction(engine, Value(this), L"/", ii, simpleFn(&intDiv))));
		add(steal(inlinedFunction(engine, Value(this), L"%", ii, simpleFn(&intMod))));
		add(steal(inlinedFunction(engine, b, L"==", ii, simpleFn(&intCmp<ifEqual>))));
		add(steal(inlinedFunction(engine, b, L"!=", ii, simpleFn(&intCmp<ifNotEqual>))));
		add(steal(inlinedFunction(engine, b, L"<", ii, simpleFn(&intCmp<ifLess>))));
		add(steal(inlinedFunction(engine, b, L">", ii, simpleFn(&intCmp<ifGreater>))));
		add(steal(inlinedFunction(engine, b, L"<=", ii, simpleFn(&intCmp<ifLessEqual>))));
		add(steal(inlinedFunction(engine, b, L">=", ii, simpleFn(&intCmp<ifGreaterEqual>))));

		add(steal(inlinedFunction(engine, Value(natType(engine)), L"nat", i, simpleFn(&intToNat))));
		add(steal(inlinedFunction(engine, Value(byteType(engine)), L"byte", i, simpleFn(&intToByte))));
		add(steal(inlinedFunction(engine, Value(floatType(engine)), L"float", i, simpleFn(&intToFloat))));

		add(steal(inlinedFunction(engine, Value(this), L"*++", r, simpleFn(&intPostfixInc))));
		add(steal(inlinedFunction(engine, Value(this), L"++*", r, simpleFn(&intPrefixInc))));
		add(steal(inlinedFunction(engine, Value(this), L"*--", r, simpleFn(&intPostfixDec))));
		add(steal(inlinedFunction(engine, Value(this), L"--*", r, simpleFn(&intPrefixDec))));

		vector<Value> ri(2);
		ri[0] = Value(this, true);
		ri[1] = Value(this);
		add(steal(inlinedFunction(engine, Value(this, true), L"=", ri, simpleFn(&intAssign))));
		add(steal(inlinedFunction(engine, Value(this), L"+=", ri, simpleFn(&intInc))));
		add(steal(inlinedFunction(engine, Value(this), L"-=", ri, simpleFn(&intDec))));

		vector<Value> rr(2, Value(this, true));
		add(steal(inlinedFunction(engine, Value(), Type::CTOR, rr, simpleFn(&intCopyCtor))));

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

	static void natAdd(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << mov(result, p.params[0]);
			p.state->to << add(result, p.params[1]);
		}
	}

	static void natInc(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << add(intRel(ptrA), p.params[1]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), intRel(ptrA));
	}

	static void natDec(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << sub(intRel(ptrA), p.params[1]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), intRel(ptrA));
	}

	static void natPrefixInc(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << add(intRel(ptrA), natConst(1));
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), intRel(ptrA));
	}

	static void natPostfixInc(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), intRel(ptrA));
		p.state->to << add(intRel(ptrA), natConst(1));
	}

	static void natPrefixDec(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << sub(intRel(ptrA), natConst(1));
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), intRel(ptrA));
	}

	static void natPostfixDec(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), intRel(ptrA));
		p.state->to << sub(intRel(ptrA), natConst(1));
	}

	static void natSub(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << mov(result, p.params[0]);
			p.state->to << sub(result, p.params[1]);
		}
	}

	static void natMul(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << mov(result, p.params[0]);
			p.state->to << mul(result, p.params[1]);
		}
	}

	static void natDiv(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << mov(result, p.params[0]);
			p.state->to << udiv(result, p.params[1]);
		}
	}

	static void natMod(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << mov(result, p.params[0]);
			p.state->to << umod(result, p.params[1]);
		}
	}

	template <CondFlag f>
	static void natCmp(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << cmp(p.params[0], p.params[1]);
			p.state->to << setCond(result, f);
		}
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

	static void natToInt(InlinedParams p) {
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), p.params[0]);
	}

	static void natToByte(InlinedParams p) {
		if (p.result->needed()) {
			p.state->to << lea(ptrA, p.params[0]);
			p.state->to << mov(p.result->location(p.state).var(), byteRel(ptrA));
		}
	}

	static void natFromByte(Nat *to, Byte f) {
		*to = f;
	}

	NatType::NatType() : Type(L"Nat", typeValue | typeFinal, Size::sNat, null) {}

	bool NatType::loadAll() {
		vector<Value> r(1, Value(this, true));
		vector<Value> ii(2, Value(this));
		Value b(boolType(engine));
		add(steal(inlinedFunction(engine, Value(this), L"+", ii, simpleFn(&natAdd))));
		add(steal(inlinedFunction(engine, Value(this), L"-", ii, simpleFn(&natSub))));
		add(steal(inlinedFunction(engine, Value(this), L"*", ii, simpleFn(&natMul))));
		add(steal(inlinedFunction(engine, Value(this), L"/", ii, simpleFn(&natDiv))));
		add(steal(inlinedFunction(engine, Value(this), L"%", ii, simpleFn(&natMod))));
		add(steal(inlinedFunction(engine, b, L"==", ii, simpleFn(&natCmp<ifEqual>))));
		add(steal(inlinedFunction(engine, b, L"!=", ii, simpleFn(&natCmp<ifNotEqual>))));
		add(steal(inlinedFunction(engine, b, L"<", ii, simpleFn(&natCmp<ifBelow>))));
		add(steal(inlinedFunction(engine, b, L">", ii, simpleFn(&natCmp<ifAbove>))));
		add(steal(inlinedFunction(engine, b, L"<=", ii, simpleFn(&natCmp<ifBelowEqual>))));
		add(steal(inlinedFunction(engine, b, L">=", ii, simpleFn(&natCmp<ifAboveEqual>))));
		add(steal(inlinedFunction(engine, Value(intType(engine)), L"int", valList(1, Value(this)), simpleFn(&natToInt))));
		add(steal(inlinedFunction(engine, Value(byteType(engine)), L"byte", valList(1, Value(this)), simpleFn(&natToByte))));

		add(steal(inlinedFunction(engine, Value(this), L"*++", r, simpleFn(&natPostfixInc))));
		add(steal(inlinedFunction(engine, Value(this), L"++*", r, simpleFn(&natPrefixInc))));
		add(steal(inlinedFunction(engine, Value(this), L"*--", r, simpleFn(&natPostfixDec))));
		add(steal(inlinedFunction(engine, Value(this), L"--*", r, simpleFn(&natPrefixDec))));

		vector<Value> ri(2);
		ri[0] = Value(this, true);
		ri[1] = Value(this);
		add(steal(inlinedFunction(engine, Value(this, true), L"=", ri, simpleFn(&natAssign))));
		add(steal(inlinedFunction(engine, Value(this), L"+=", ri, simpleFn(&natInc))));
		add(steal(inlinedFunction(engine, Value(this), L"-=", ri, simpleFn(&natDec))));

		vector<Value> rr(2, Value(this, true));
		add(steal(inlinedFunction(engine, Value(), Type::CTOR, rr, simpleFn(&natCopyCtor))));

		// TODO? Inline?
		vector<Value> rb = valList(2, Value(this, true), Value(byteType(engine)));
		Auto<Function> f = nativeFunction(engine, Value(), Type::CTOR, rb, &natFromByte);
		f->flags |= namedAutoCast;
		add(f);

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

	static void byteAdd(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << mov(result, p.params[0]);
			p.state->to << add(result, p.params[1]);
		}
	}

	static void byteInc(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << add(byteRel(ptrA), p.params[1]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), byteRel(ptrA));
	}

	static void byteDec(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << sub(byteRel(ptrA), p.params[1]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), byteRel(ptrA));
	}

	static void bytePrefixInc(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << add(byteRel(ptrA), byteConst(1));
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), byteRel(ptrA));
	}

	static void bytePostfixInc(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), byteRel(ptrA));
		p.state->to << add(byteRel(ptrA), byteConst(1));
	}

	static void bytePrefixDec(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << sub(byteRel(ptrA), byteConst(1));
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), byteRel(ptrA));
	}

	static void bytePostfixDec(InlinedParams p) {
		p.state->to << mov(ptrA, p.params[0]);
		if (p.result->needed())
			p.state->to << mov(p.result->location(p.state).var(), byteRel(ptrA));
		p.state->to << sub(byteRel(ptrA), byteConst(1));
	}

	static void byteSub(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << mov(result, p.params[0]);
			p.state->to << sub(result, p.params[1]);
		}
	}

	template <CondFlag f>
	static void byteCmp(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << cmp(p.params[0], p.params[1]);
			p.state->to << setCond(result, f);
		}
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
		vector<Value> ii(2, Value(this));
		Value b(boolType(engine));
		add(steal(inlinedFunction(engine, Value(this), L"+", ii, simpleFn(&byteAdd))));
		add(steal(inlinedFunction(engine, Value(this), L"-", ii, simpleFn(&byteSub))));
		// add(steal(inlinedFunction(engine, Value(this), L"*", ii, simpleFn(&byteMul))));
		add(steal(inlinedFunction(engine, b, L"==", ii, simpleFn(&byteCmp<ifEqual>))));
		add(steal(inlinedFunction(engine, b, L"!=", ii, simpleFn(&byteCmp<ifNotEqual>))));
		add(steal(inlinedFunction(engine, b, L"<", ii, simpleFn(&byteCmp<ifBelow>))));
		add(steal(inlinedFunction(engine, b, L">", ii, simpleFn(&byteCmp<ifAbove>))));
		add(steal(inlinedFunction(engine, b, L"<=", ii, simpleFn(&byteCmp<ifBelowEqual>))));
		add(steal(inlinedFunction(engine, b, L">=", ii, simpleFn(&byteCmp<ifAboveEqual>))));

		add(steal(inlinedFunction(engine, Value(this), L"*++", r, simpleFn(&bytePostfixInc))));
		add(steal(inlinedFunction(engine, Value(this), L"++*", r, simpleFn(&bytePrefixInc))));
		add(steal(inlinedFunction(engine, Value(this), L"*--", r, simpleFn(&bytePostfixDec))));
		add(steal(inlinedFunction(engine, Value(this), L"--*", r, simpleFn(&bytePrefixDec))));

		vector<Value> ri(2);
		ri[0] = Value(this, true);
		ri[1] = Value(this);
		add(steal(inlinedFunction(engine, Value(this, true), L"=", ri, simpleFn(&byteAssign))));
		add(steal(inlinedFunction(engine, Value(this), L"+=", ri, simpleFn(&byteInc))));
		add(steal(inlinedFunction(engine, Value(this), L"-=", ri, simpleFn(&byteDec))));

		vector<Value> rr(2, Value(this, true));
		add(steal(inlinedFunction(engine, Value(), Type::CTOR, rr, simpleFn(&byteCopyCtor))));

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
