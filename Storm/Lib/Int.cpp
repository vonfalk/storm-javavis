#include "stdafx.h"
#include "Int.h"
#include "Engine.h"
#include "Code.h"
#include "Function.h"

namespace storm {

	static void intAdd(InlinedParams p) {
		p.state.to << code::mov(p.result, p.params[0]);
		p.state.to << code::add(p.result, p.params[1]);
	}

	static void intSub(InlinedParams p) {
		p.state.to << code::mov(p.result, p.params[0]);
		p.state.to << code::sub(p.result, p.params[1]);
	}

	static Int add(Int a, Int b) {
		return a + b;
	}


	IntType::IntType() : Type(L"Int", typeValue | typeFinal, sizeof(code::Int)) {
		vector<Value> ii(2, Value(this));
		add(inlinedFunction(engine, Value(this), L"+", ii, simpleFn(&intAdd)));
		add(inlinedFunction(engine, Value(this), L"-", ii, simpleFn(&intSub)));
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
