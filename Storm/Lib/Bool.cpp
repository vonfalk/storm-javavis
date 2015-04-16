#include "stdafx.h"
#include "Bool.h"
#include "Type.h"
#include "Bool.h"
#include "Engine.h"
#include "Code.h"
#include "Function.h"

namespace storm {

	static void boolAnd(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << code::mov(result, p.params[0]);
			p.state->to << code::and(result, p.params[1]);
		}
	}

	static void boolOr(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << code::mov(result, p.params[0]);
			p.state->to << code::or(result, p.params[1]);
		}
	}

	static void boolEq(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << code::cmp(p.params[0], p.params[1]);
			p.state->to << code::setCond(result, code::ifEqual);
		}
	}

	static void boolNeq(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << code::cmp(p.params[0], p.params[1]);
			p.state->to << code::setCond(result, code::ifNotEqual);
		}
	}

	static void boolNot(InlinedParams p) {
		if (p.result->needed()) {
			code::Value result = p.result->location(p.state).var();
			p.state->to << code::cmp(p.params[0], code::byteConst(0));
			p.state->to << code::setCond(result, code::ifEqual);
		}
	}

	static void boolAssign(InlinedParams p) {
		p.state->to << code::mov(code::ptrA, p.params[0]);
		p.state->to << code::mov(code::byteRel(code::ptrA), p.params[1]);
		if (p.result->needed()) {
			if (p.result->type().ref) {
				if (!p.result->suggest(p.state, p.params[0]))
					p.state->to << code::mov(p.result->location(p.state).var(), code::ptrA);
			} else {
				if (!p.result->suggest(p.state, p.params[1]))
					p.state->to << code::mov(p.result->location(p.state).var(), p.params[1]);
			}
		}
	}

	static void boolCopyCtor(InlinedParams p) {
		p.state->to << code::mov(code::ptrC, p.params[1]);
		p.state->to << code::mov(code::ptrA, p.params[0]);
		p.state->to << code::mov(code::byteRel(code::ptrA), code::byteRel(code::ptrC));
	}

	BoolType::BoolType() : Type(L"Bool", typeValue, Size::sByte, null) {
		vector<Value> bb(2, Value(this));
		vector<Value> b(1, Value(this));
		add(steal(inlinedFunction(engine, Value(this), L"&", bb, simpleFn(&boolAnd))));
		add(steal(inlinedFunction(engine, Value(this), L"|", bb, simpleFn(&boolOr))));
		add(steal(inlinedFunction(engine, Value(this), L"==", bb, simpleFn(&boolEq))));
		add(steal(inlinedFunction(engine, Value(this), L"!=", bb, simpleFn(&boolNeq))));
		add(steal(inlinedFunction(engine, Value(this), L"!", b, simpleFn(&boolNot))));

		vector<Value> rr(2, Value(this, true));
		add(steal(inlinedFunction(engine, Value(), Type::CTOR, rr, simpleFn(&boolCopyCtor))));

		vector<Value> ri(2);
		ri[0] = Value(this, true);
		ri[1] = Value(this);
		add(steal(inlinedFunction(engine, Value(this, true), L"=", ri, simpleFn(&boolAssign))));
	}

	Type *boolType(Engine &e) {
		Type *t = e.specialBuiltIn(specialBool);
		if (!t) {
			t = CREATE(BoolType, e);
			e.setSpecialBuiltIn(specialBool, t);
		}
		return t;
	}

	Str *toS(EnginePtr e, Bool v) {
		if (v) {
			return CREATE(Str, e.v, L"true");
		} else {
			return CREATE(Str, e.v, L"false");
		}
	}

}
