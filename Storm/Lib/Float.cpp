#include "stdafx.h"
#include "Float.h"
#include "Code.h"
#include "CodeGen.h"
#include "Function.h"
#include "Engine.h"

namespace storm {
	using namespace code;

	static void floatCopyCtor(InlinedParams p) {
		p.state->to << mov(ptrC, p.params[1]);
		p.state->to << mov(ptrA, p.params[0]);
		p.state->to << mov(floatRel(ptrA), floatRel(ptrC));
	}

	static void floatAdd(InlinedParams p) {
		p.state->to << fld(p.params[0]);
		p.state->to << fld(p.params[1]);
		p.state->to << faddp();
		p.state->to << fstp(p.result->location(p.state).var());
		p.state->to << fwait();
	}

	static void floatSub(InlinedParams p) {
		p.state->to << fld(p.params[0]);
		p.state->to << fld(p.params[1]);
		p.state->to << fsubp();
		p.state->to << fstp(p.result->location(p.state).var());
		p.state->to << fwait();
	}

	static void floatMul(InlinedParams p) {
		p.state->to << fld(p.params[0]);
		p.state->to << fld(p.params[1]);
		p.state->to << fmulp();
		p.state->to << fstp(p.result->location(p.state).var());
		p.state->to << fwait();
	}

	static void floatDiv(InlinedParams p) {
		p.state->to << fld(p.params[0]);
		p.state->to << fld(p.params[1]);
		p.state->to << fdivp();
		p.state->to << fstp(p.result->location(p.state).var());
		p.state->to << fwait();
	}

	static void floatToInt(InlinedParams p) {
		if (!p.result->needed())
			return;
		p.state->to << fld(p.params[0]);
		p.state->to << fistp(p.result->location(p.state).var());
	}

	FloatType::FloatType() : Type(L"Float", typeValue | typeFinal, Size::sFloat, null) {}

	bool FloatType::loadAll() {
		vector<Value> r(1, Value(this, true));
		vector<Value> v(1, Value(this));
		vector<Value> rr(2, Value(this, true));
		vector<Value> vv(2, Value(this));
		Value b(boolType(engine));

		add(steal(inlinedFunction(engine, Value(), Type::CTOR, rr, simpleFn(&floatCopyCtor))));
		add(steal(inlinedFunction(engine, Value(this), L"+", vv, simpleFn(&floatAdd))));
		add(steal(inlinedFunction(engine, Value(this), L"-", vv, simpleFn(&floatSub))));
		add(steal(inlinedFunction(engine, Value(this), L"*", vv, simpleFn(&floatMul))));
		add(steal(inlinedFunction(engine, Value(this), L"/", vv, simpleFn(&floatDiv))));

		add(steal(inlinedFunction(engine, Value(intType(engine)), L"int", v, simpleFn(&floatToInt))));


		return Type::loadAll();
	}

	Type *floatType(Engine &e) {
		Type *t = e.specialBuiltIn(specialFloat);
		if (!t) {
			t = CREATE(FloatType, e);
			e.setSpecialBuiltIn(specialFloat, t);
		}
		return t;
	}

	Str *toS(EnginePtr e, Float v) {
		return CREATE(Str, e.v, ::toS(v));
	}

}
