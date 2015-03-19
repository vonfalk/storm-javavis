#include "stdafx.h"
#include "Int.h"
#include "Engine.h"
#include "Code.h"
#include "Function.h"

namespace storm {

	static void intAdd(InlinedParams p) {
		if (p.result.needed()) {
			code::Value result = p.result.location(p.state).var;
			p.state.to << code::mov(result, p.params[0]);
			p.state.to << code::add(result, p.params[1]);
		}
	}

	static void intPrefixInc(InlinedParams p) {
		p.state.to << code::mov(code::ptrA, p.params[0]);
		p.state.to << code::add(intRel(code::ptrA), code::intConst(1));
		if (p.result.needed())
			p.state.to << code::mov(p.result.location(p.state).var, intRel(code::ptrA));
	}

	static void intPostfixInc(InlinedParams p) {
		p.state.to << code::mov(code::ptrA, p.params[0]);
		if (p.result.needed())
			p.state.to << code::mov(p.result.location(p.state).var, intRel(code::ptrA));
		p.state.to << code::add(intRel(code::ptrA), code::intConst(1));
	}

	static void intPrefixDec(InlinedParams p) {
		p.state.to << code::mov(code::ptrA, p.params[0]);
		p.state.to << code::sub(intRel(code::ptrA), code::intConst(1));
		if (p.result.needed())
			p.state.to << code::mov(p.result.location(p.state).var, intRel(code::ptrA));
	}

	static void intPostfixDec(InlinedParams p) {
		p.state.to << code::mov(code::ptrA, p.params[0]);
		if (p.result.needed())
			p.state.to << code::mov(p.result.location(p.state).var, intRel(code::ptrA));
		p.state.to << code::sub(intRel(code::ptrA), code::intConst(1));
	}

	static void intSub(InlinedParams p) {
		if (p.result.needed()) {
			code::Value result = p.result.location(p.state).var;
			p.state.to << code::mov(result, p.params[0]);
			p.state.to << code::sub(result, p.params[1]);
		}
	}

	static void intMul(InlinedParams p) {
		if (p.result.needed()) {
			code::Value result = p.result.location(p.state).var;
			p.state.to << code::mov(result, p.params[0]);
			p.state.to << code::mul(result, p.params[1]);
		}
	}

	template <code::CondFlag f>
	static void intCmp(InlinedParams p) {
		if (p.result.needed()) {
			code::Value result = p.result.location(p.state).var;
			p.state.to << code::cmp(p.params[0], p.params[1]);
			p.state.to << code::setCond(result, f);
		}
	}

	static void intAssign(InlinedParams p) {
		p.state.to << code::mov(code::ptrA, p.params[0]);
		p.state.to << code::mov(code::intRel(code::ptrA), p.params[1]);
		if (p.result.needed()) {
			if (p.result.type.ref) {
				if (!p.result.suggest(p.state, p.params[0]))
					p.state.to << code::mov(p.result.location(p.state).var, code::ptrA);
			} else {
				if (!p.result.suggest(p.state, p.params[1]))
					p.state.to << code::mov(p.result.location(p.state).var, p.params[1]);
			}
		}
	}

	static void intCopyCtor(InlinedParams p) {
		p.state.to << code::mov(code::ptrC, p.params[1]);
		p.state.to << code::mov(code::ptrA, p.params[0]);
		p.state.to << code::mov(code::intRel(code::ptrA), code::intRel(code::ptrC));
	}


	static void intToNat(InlinedParams p) {
		if (p.result.needed())
			p.state.to << code::mov(p.result.location(p.state).var, p.params[0]);
	}

	IntType::IntType() : Type(L"Int", typeValue | typeFinal, Size::sInt, null) {}

	void IntType::lazyLoad() {
		vector<Value> r(1, Value(this, true));
		vector<Value> ii(2, Value(this));
		Value b(boolType(engine));
		add(steal(inlinedFunction(engine, Value(this), L"+", ii, simpleFn(&intAdd))));
		add(steal(inlinedFunction(engine, Value(this), L"-", ii, simpleFn(&intSub))));
		add(steal(inlinedFunction(engine, Value(this), L"*", ii, simpleFn(&intMul))));
		add(steal(inlinedFunction(engine, b, L"==", ii, simpleFn(&intCmp<code::ifEqual>))));
		add(steal(inlinedFunction(engine, b, L"!=", ii, simpleFn(&intCmp<code::ifNotEqual>))));
		add(steal(inlinedFunction(engine, b, L"<", ii, simpleFn(&intCmp<code::ifLess>))));
		add(steal(inlinedFunction(engine, b, L">", ii, simpleFn(&intCmp<code::ifGreater>))));
		add(steal(inlinedFunction(engine, b, L"<=", ii, simpleFn(&intCmp<code::ifLessEqual>))));
		add(steal(inlinedFunction(engine, b, L">=", ii, simpleFn(&intCmp<code::ifGreaterEqual>))));

		add(steal(inlinedFunction(engine, Value(natType(engine)), L"nat", valList(1, Value(this)), simpleFn(&intToNat))));

		add(steal(inlinedFunction(engine, Value(this), L"*++", r, simpleFn(&intPostfixInc))));
		add(steal(inlinedFunction(engine, Value(this), L"++*", r, simpleFn(&intPrefixInc))));
		add(steal(inlinedFunction(engine, Value(this), L"*--", r, simpleFn(&intPostfixDec))));
		add(steal(inlinedFunction(engine, Value(this), L"--*", r, simpleFn(&intPrefixDec))));

		vector<Value> ri(2);
		ri[0] = Value(this, true);
		ri[1] = Value(this);
		add(steal(inlinedFunction(engine, Value(this, true), L"=", ri, simpleFn(&intAssign))));

		vector<Value> rr(2, Value(this, true));
		add(steal(inlinedFunction(engine, Value(), Type::CTOR, rr, simpleFn(&intCopyCtor))));
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
		if (p.result.needed()) {
			code::Value result = p.result.location(p.state).var;
			p.state.to << code::mov(result, p.params[0]);
			p.state.to << code::add(result, p.params[1]);
		}
	}

	static void natPrefixInc(InlinedParams p) {
		p.state.to << code::mov(code::ptrA, p.params[0]);
		p.state.to << code::add(intRel(code::ptrA), code::natConst(1));
		if (p.result.needed())
			p.state.to << code::mov(p.result.location(p.state).var, intRel(code::ptrA));
	}

	static void natPostfixInc(InlinedParams p) {
		p.state.to << code::mov(code::ptrA, p.params[0]);
		if (p.result.needed())
			p.state.to << code::mov(p.result.location(p.state).var, intRel(code::ptrA));
		p.state.to << code::add(intRel(code::ptrA), code::natConst(1));
	}

	static void natPrefixDec(InlinedParams p) {
		p.state.to << code::mov(code::ptrA, p.params[0]);
		p.state.to << code::sub(intRel(code::ptrA), code::natConst(1));
		if (p.result.needed())
			p.state.to << code::mov(p.result.location(p.state).var, intRel(code::ptrA));
	}

	static void natPostfixDec(InlinedParams p) {
		p.state.to << code::mov(code::ptrA, p.params[0]);
		if (p.result.needed())
			p.state.to << code::mov(p.result.location(p.state).var, intRel(code::ptrA));
		p.state.to << code::sub(intRel(code::ptrA), code::natConst(1));
	}

	static void natSub(InlinedParams p) {
		if (p.result.needed()) {
			code::Value result = p.result.location(p.state).var;
			p.state.to << code::mov(result, p.params[0]);
			p.state.to << code::sub(result, p.params[1]);
		}
	}

	template <code::CondFlag f>
	static void natCmp(InlinedParams p) {
		if (p.result.needed()) {
			code::Value result = p.result.location(p.state).var;
			p.state.to << code::cmp(p.params[0], p.params[1]);
			p.state.to << code::setCond(result, f);
		}
	}

	static void natAssign(InlinedParams p) {
		p.state.to << code::mov(code::ptrA, p.params[0]);
		p.state.to << code::mov(code::intRel(code::ptrA), p.params[1]);
		if (p.result.needed()) {
			if (p.result.type.ref) {
				if (!p.result.suggest(p.state, p.params[0]))
					p.state.to << code::mov(p.result.location(p.state).var, code::ptrA);
			} else {
				if (!p.result.suggest(p.state, p.params[1]))
					p.state.to << code::mov(p.result.location(p.state).var, p.params[1]);
			}
		}
	}

	static void natCopyCtor(InlinedParams p) {
		p.state.to << code::mov(code::ptrC, p.params[1]);
		p.state.to << code::mov(code::ptrA, p.params[0]);
		p.state.to << code::mov(code::intRel(code::ptrA), code::intRel(code::ptrC));
	}

	static void natToInt(InlinedParams p) {
		if (p.result.needed())
			p.state.to << code::mov(p.result.location(p.state).var, p.params[0]);
	}

	NatType::NatType() : Type(L"Nat", typeValue | typeFinal, Size::sNat, null) {}

	void NatType::lazyLoad() {
		vector<Value> r(1, Value(this, true));
		vector<Value> ii(2, Value(this));
		Value b(boolType(engine));
		add(steal(inlinedFunction(engine, Value(this), L"+", ii, simpleFn(&natAdd))));
		add(steal(inlinedFunction(engine, Value(this), L"-", ii, simpleFn(&natSub))));
		// add(steal(inlinedFunction(engine, Value(this), L"*", ii, simpleFn(&natMul))));
		add(steal(inlinedFunction(engine, b, L"==", ii, simpleFn(&natCmp<code::ifEqual>))));
		add(steal(inlinedFunction(engine, b, L"!=", ii, simpleFn(&natCmp<code::ifNotEqual>))));
		add(steal(inlinedFunction(engine, b, L"<", ii, simpleFn(&natCmp<code::ifBelow>))));
		add(steal(inlinedFunction(engine, b, L">", ii, simpleFn(&natCmp<code::ifAbove>))));
		add(steal(inlinedFunction(engine, b, L"<=", ii, simpleFn(&natCmp<code::ifBelowEqual>))));
		add(steal(inlinedFunction(engine, b, L">=", ii, simpleFn(&natCmp<code::ifAboveEqual>))));
		add(steal(inlinedFunction(engine, Value(intType(engine)), L"int", valList(1, Value(this)), simpleFn(&natToInt))));

		add(steal(inlinedFunction(engine, Value(this), L"*++", r, simpleFn(&natPostfixInc))));
		add(steal(inlinedFunction(engine, Value(this), L"++*", r, simpleFn(&natPrefixInc))));
		add(steal(inlinedFunction(engine, Value(this), L"*--", r, simpleFn(&natPostfixDec))));
		add(steal(inlinedFunction(engine, Value(this), L"--*", r, simpleFn(&natPrefixDec))));

		vector<Value> ri(2);
		ri[0] = Value(this, true);
		ri[1] = Value(this);
		add(steal(inlinedFunction(engine, Value(this, true), L"=", ri, simpleFn(&natAssign))));

		vector<Value> rr(2, Value(this, true));
		add(steal(inlinedFunction(engine, Value(), Type::CTOR, rr, simpleFn(&intCopyCtor))));
	}


	Type *natType(Engine &e) {
		Type *t = e.specialBuiltIn(specialNat);
		if (!t) {
			t = CREATE(NatType, e);
			e.setSpecialBuiltIn(specialNat, t);
		}
		return t;
	}

}
