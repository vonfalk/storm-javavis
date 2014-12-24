#include "stdafx.h"
#include "Bool.h"
#include "Type.h"
#include "BoolDef.h"
#include "Engine.h"
#include "Code.h"
#include "Function.h"

namespace storm {

	static void boolAnd(InlinedParams p) {
		if (p.result.needed()) {
			code::Value result = p.result.location(p.state);
			p.state.to << code::mov(result, p.params[0]);
			p.state.to << code::and(result, p.params[1]);
		}
	}

	static void boolOr(InlinedParams p) {
		if (p.result.needed()) {
			code::Value result = p.result.location(p.state);
			p.state.to << code::mov(result, p.params[0]);
			p.state.to << code::or(result, p.params[1]);
		}
	}

	static void boolEq(InlinedParams p) {
		if (p.result.needed()) {
			code::Value result = p.result.location(p.state);
			p.state.to << code::cmp(p.params[0], p.params[1]);
			p.state.to << code::setCond(result, code::ifEqual);
		}
	}

	static void boolNeq(InlinedParams p) {
		if (p.result.needed()) {
			code::Value result = p.result.location(p.state);
			p.state.to << code::cmp(p.params[0], p.params[1]);
			p.state.to << code::setCond(result, code::ifNotEqual);
		}
	}

	static void boolNot(InlinedParams p) {
		if (p.result.needed()) {
			code::Value result = p.result.location(p.state);
			p.state.to << code::cmp(p.params[0], code::byteConst(0));
			p.state.to << code::setCond(result, code::ifEqual);
		}
	}

	BoolType::BoolType() : Type(L"Bool", typeValue, sizeof(Bool)) {
		vector<Value> bb(2, Value(this));
		vector<Value> b(1, Value(this));
		add(inlinedFunction(engine, Value(this), L"&", bb, simpleFn(&boolAnd)));
		add(inlinedFunction(engine, Value(this), L"|", bb, simpleFn(&boolOr)));
		add(inlinedFunction(engine, Value(this), L"==", bb, simpleFn(&boolEq)));
		add(inlinedFunction(engine, Value(this), L"!=", bb, simpleFn(&boolNeq)));
		add(inlinedFunction(engine, Value(this), L"!", b, simpleFn(&boolNot)));
	}

	Type *boolType(Engine &e) {
		Type *t = e.specialBuiltIn(specialBool);
		if (!t) {
			t = CREATE(BoolType, e);
			e.setSpecialBuiltIn(specialBool, t);
		}
		return t;
	}

}
