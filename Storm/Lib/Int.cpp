#include "stdafx.h"
#include "Int.h"
#include "Engine.h"
#include "Code.h"
#include "Function.h"

namespace storm {

	static void intAdd(InlinedParams p) {
		if (p.result.needed()) {
			code::Value result = p.result.location(p.state);
			p.state.to << code::mov(result, p.params[0]);
			p.state.to << code::add(result, p.params[1]);
		}
	}

	static void intSub(InlinedParams p) {
		if (p.result.needed()) {
			code::Value result = p.result.location(p.state);
			p.state.to << code::mov(result, p.params[0]);
			p.state.to << code::sub(result, p.params[1]);
		}
	}

	static void intMul(InlinedParams p) {
		if (p.result.needed()) {
			code::Value result = p.result.location(p.state);
			p.state.to << code::mov(result, p.params[0]);
			p.state.to << code::mul(result, p.params[1]);
		}
	}

	template <code::CondFlag f>
	static void intCmp(InlinedParams p) {
		if (p.result.needed()) {
			code::Value result = p.result.location(p.state);
			p.state.to << code::cmp(p.params[0], p.params[1]);
			p.state.to << code::setCond(result, f);
		}
	}

	static void intAssign(InlinedParams p) {
		p.state.to << code::mov(code::ptrA, p.params[0]);
		p.state.to << code::mov(code::intRel(code::ptrA, 0), p.params[1]);
		if (p.result.needed()) {
			if (p.result.type.ref) {
				if (!p.result.suggest(p.state, p.params[0]))
					p.state.to << code::mov(p.result.location(p.state), code::ptrA);
			} else {
				if (!p.result.suggest(p.state, p.params[1]))
					p.state.to << code::mov(p.result.location(p.state), p.params[1]);
			}
		}
	}

	IntType::IntType() : Type(L"Int", typeValue | typeFinal, sizeof(code::Int)) {
		vector<Value> ii(2, Value(this));
		Value b(boolType(engine));
		add(inlinedFunction(engine, Value(this), L"+", ii, simpleFn(&intAdd)));
		add(inlinedFunction(engine, Value(this), L"-", ii, simpleFn(&intSub)));
		add(inlinedFunction(engine, Value(this), L"*", ii, simpleFn(&intMul)));
		add(inlinedFunction(engine, b, L"==", ii, simpleFn(&intCmp<code::ifEqual>)));
		add(inlinedFunction(engine, b, L"!=", ii, simpleFn(&intCmp<code::ifNotEqual>)));
		add(inlinedFunction(engine, b, L"<", ii, simpleFn(&intCmp<code::ifLess>)));
		add(inlinedFunction(engine, b, L">", ii, simpleFn(&intCmp<code::ifGreater>)));
		add(inlinedFunction(engine, b, L"<=", ii, simpleFn(&intCmp<code::ifLessEqual>)));
		add(inlinedFunction(engine, b, L">=", ii, simpleFn(&intCmp<code::ifGreaterEqual>)));

		vector<Value> ri(2);
		ri[0] = Value(this, true);
		ri[1] = Value(this);
		add(inlinedFunction(engine, Value(this, true), L"=", ri, simpleFn(&intAssign)));
	}


	Type *intType(Engine &e) {
		Type *t = e.specialBuiltIn(specialInt);
		if (!t) {
			t = CREATE(IntType, e);
			e.setSpecialBuiltIn(specialInt, t);
		}
		return t;
	}

	NatType::NatType() : Type(L"Nat", typeValue | typeFinal, sizeof(code::Nat)) {}

	Type *natType(Engine &e) {
		Type *t = e.specialBuiltIn(specialNat);
		if (!t) {
			t = CREATE(NatType, e);
			e.setSpecialBuiltIn(specialNat, t);
		}
		return t;
	}

}
