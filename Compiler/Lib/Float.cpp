#include "stdafx.h"
#include "Float.h"
#include "Function.h"
#include "Number.h"

namespace storm {
	using namespace code;

	Type *createFloat(Str *name, Size size, GcType *type) {
		return new (name) FloatType(name, type);
	}

	static void floatCopyCtor(InlineParams p) {
		*p.state->to << mov(ptrC, p.params->at(1));
		*p.state->to << mov(ptrA, p.params->at(0));
		*p.state->to << mov(floatRel(ptrA, Offset()), floatRel(ptrC, Offset()));
	}

	static void floatAdd(InlineParams p) {
		if (p.result->needed()) {
			*p.state->to << fld(p.params->at(0));
			*p.state->to << fld(p.params->at(1));
			*p.state->to << faddp();
			*p.state->to << fstp(p.result->location(p.state).v);
			*p.state->to << fwait();
		}
	}

	static void floatSub(InlineParams p) {
		if (p.result->needed()) {
			*p.state->to << fld(p.params->at(0));
			*p.state->to << fld(p.params->at(1));
			*p.state->to << fsubp();
			*p.state->to << fstp(p.result->location(p.state).v);
			*p.state->to << fwait();
		}
	}

	static void floatMul(InlineParams p) {
		if (p.result->needed()) {
			*p.state->to << fld(p.params->at(0));
			*p.state->to << fld(p.params->at(1));
			*p.state->to << fmulp();
			*p.state->to << fstp(p.result->location(p.state).v);
			*p.state->to << fwait();
		}
	}

	static void floatDiv(InlineParams p) {
		if (p.result->needed()) {
			*p.state->to << fld(p.params->at(0));
			*p.state->to << fld(p.params->at(1));
			*p.state->to << fdivp();
			*p.state->to << fstp(p.result->location(p.state).v);
			*p.state->to << fwait();
		}
	}

	static void floatAssign(InlineParams p) {
		*p.state->to << mov(ptrA, p.params->at(0));
		*p.state->to << mov(floatRel(ptrA, Offset()), p.params->at(1));
		if (p.result->needed())
			if (!p.result->suggest(p.state, p.params->at(0)))
				*p.state->to << mov(p.result->location(p.state).v, floatRel(ptrA, Offset()));
	}

	static void floatToInt(InlineParams p) {
		if (!p.result->needed())
			return;
		*p.state->to << fld(p.params->at(0));
		*p.state->to << fistp(p.result->location(p.state).v);
	}

	template <CondFlag f>
	static void floatCmp(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->to << fld(p.params->at(1));
			*p.state->to << fld(p.params->at(0));
			*p.state->to << fcompp();
			*p.state->to << setCond(result, f);
		}
	}

	FloatType::FloatType(Str *name, GcType *type) : Type(name, typeValue | typeFinal, Size::sFloat, type, null) {}

	Bool FloatType::loadAll() {
		Array<Value> *r = new (this) Array<Value>(1, Value(this, true));
		Array<Value> *v = new (this) Array<Value>(1, Value(this, false));
		Array<Value> *rr = new (this) Array<Value>(2, Value(this, true));
		Array<Value> *vv = new (this) Array<Value>(2, Value(this, false));
		Array<Value> *rv = new (this) Array<Value>(2, Value(this, true));
		rv->at(1) = Value(this);

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &floatCopyCtor)));
		add(inlinedFunction(engine, Value(this, true), L"=", rv, fnPtr(engine, &floatAssign)));

		add(inlinedFunction(engine, Value(this), L"+", vv, fnPtr(engine, &floatAdd)));
		add(inlinedFunction(engine, Value(this), L"-", vv, fnPtr(engine, &floatSub)));
		add(inlinedFunction(engine, Value(this), L"*", vv, fnPtr(engine, &floatMul)));
		add(inlinedFunction(engine, Value(this), L"/", vv, fnPtr(engine, &floatDiv)));

		Value vBool = Value(StormInfo<Bool>::type(engine));
		add(inlinedFunction(engine, vBool, L">", vv, fnPtr(engine, &floatCmp<ifFAbove>)));
		add(inlinedFunction(engine, vBool, L">=", vv, fnPtr(engine, &floatCmp<ifFAboveEqual>)));
		add(inlinedFunction(engine, vBool, L"<", vv, fnPtr(engine, &floatCmp<ifFBelow>)));
		add(inlinedFunction(engine, vBool, L"<=", vv, fnPtr(engine, &floatCmp<ifFBelowEqual>)));
		add(inlinedFunction(engine, vBool, L"==", vv, fnPtr(engine, &floatCmp<ifEqual>)));
		add(inlinedFunction(engine, vBool, L"!=", vv, fnPtr(engine, &floatCmp<ifNotEqual>)));

		Value vInt = Value(StormInfo<Int>::type(engine));
		add(inlinedFunction(engine, vInt, L"int", v, fnPtr(engine, &floatToInt)));

		add(nativeFunction(engine, Value(this), L"min", vv, address(&numMin<float>)));
		add(nativeFunction(engine, Value(this), L"max", vv, address(&numMax<float>)));

		return Type::loadAll();
	}

}
