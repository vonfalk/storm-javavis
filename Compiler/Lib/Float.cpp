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
		*p.state->l << mov(ptrC, p.params->at(1));
		*p.state->l << mov(ptrA, p.params->at(0));
		*p.state->l << mov(floatRel(ptrA, Offset()), floatRel(ptrC, Offset()));
	}

	static void floatAdd(InlineParams p) {
		if (p.result->needed()) {
			*p.state->l << fld(p.params->at(0));
			*p.state->l << fld(p.params->at(1));
			*p.state->l << faddp();
			*p.state->l << fstp(p.result->location(p.state).v);
			*p.state->l << fwait();
		}
	}

	static void floatSub(InlineParams p) {
		if (p.result->needed()) {
			*p.state->l << fld(p.params->at(0));
			*p.state->l << fld(p.params->at(1));
			*p.state->l << fsubp();
			*p.state->l << fstp(p.result->location(p.state).v);
			*p.state->l << fwait();
		}
	}

	static void floatMul(InlineParams p) {
		if (p.result->needed()) {
			*p.state->l << fld(p.params->at(0));
			*p.state->l << fld(p.params->at(1));
			*p.state->l << fmulp();
			*p.state->l << fstp(p.result->location(p.state).v);
			*p.state->l << fwait();
		}
	}

	static void floatDiv(InlineParams p) {
		if (p.result->needed()) {
			*p.state->l << fld(p.params->at(0));
			*p.state->l << fld(p.params->at(1));
			*p.state->l << fdivp();
			*p.state->l << fstp(p.result->location(p.state).v);
			*p.state->l << fwait();
		}
	}

	static void floatAssign(InlineParams p) {
		*p.state->l << mov(ptrA, p.params->at(0));
		*p.state->l << mov(floatRel(ptrA, Offset()), p.params->at(1));
		if (p.result->needed())
			if (!p.result->suggest(p.state, p.params->at(0)))
				*p.state->l << mov(p.result->location(p.state).v, floatRel(ptrA, Offset()));
	}

	static void floatToInt(InlineParams p) {
		if (!p.result->needed())
			return;
		*p.state->l << fld(p.params->at(0));
		*p.state->l << fistp(p.result->location(p.state).v);
	}

	template <CondFlag f>
	static void floatCmp(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << fld(p.params->at(1));
			*p.state->l << fld(p.params->at(0));
			*p.state->l << fcompp();
			*p.state->l << setCond(result, f);
		}
	}

	static Float floatMin(Float a, Float b) {
		return min(a, b);
	}

	static Float floatMax(Float a, Float b) {
		return max(a, b);
	}

	FloatType::FloatType(Str *name, GcType *type) : Type(name, typeValue | typeFinal, Size::sFloat, type, null) {}

	Bool FloatType::loadAll() {
		//Array<Value> *r = new (this) Array<Value>(1, Value(this, true));
		Array<Value> *v = new (this) Array<Value>(1, Value(this, false));
		Array<Value> *rr = new (this) Array<Value>(2, Value(this, true));
		Array<Value> *vv = new (this) Array<Value>(2, Value(this, false));
		Array<Value> *rv = new (this) Array<Value>(2, Value(this, true));
		rv->at(1) = Value(this);

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &floatCopyCtor)));
		add(inlinedFunction(engine, Value(this, true), S("="), rv, fnPtr(engine, &floatAssign)));

		add(inlinedFunction(engine, Value(this), S("+"), vv, fnPtr(engine, &floatAdd)));
		add(inlinedFunction(engine, Value(this), S("-"), vv, fnPtr(engine, &floatSub)));
		add(inlinedFunction(engine, Value(this), S("*"), vv, fnPtr(engine, &floatMul)));
		add(inlinedFunction(engine, Value(this), S("/"), vv, fnPtr(engine, &floatDiv)));

		Value vBool = Value(StormInfo<Bool>::type(engine));
		add(inlinedFunction(engine, vBool, S(">"), vv, fnPtr(engine, &floatCmp<ifFAbove>)));
		add(inlinedFunction(engine, vBool, S(">="), vv, fnPtr(engine, &floatCmp<ifFAboveEqual>)));
		add(inlinedFunction(engine, vBool, S("<"), vv, fnPtr(engine, &floatCmp<ifFBelow>)));
		add(inlinedFunction(engine, vBool, S("<="), vv, fnPtr(engine, &floatCmp<ifFBelowEqual>)));
		add(inlinedFunction(engine, vBool, S("=="), vv, fnPtr(engine, &floatCmp<ifEqual>)));
		add(inlinedFunction(engine, vBool, S("!="), vv, fnPtr(engine, &floatCmp<ifNotEqual>)));

		Value vInt = Value(StormInfo<Int>::type(engine));
		add(inlinedFunction(engine, vInt, S("int"), v, fnPtr(engine, &floatToInt)));

		add(nativeFunction(engine, Value(this), S("min"), vv, address(&floatMin)));
		add(nativeFunction(engine, Value(this), S("max"), vv, address(&floatMax)));

		return Type::loadAll();
	}


	Type *createDouble(Str *name, Size size, GcType *type) {
		return new (name) DoubleType(name, type);
	}

	static void doubleCopyCtor(InlineParams p) {
		*p.state->l << mov(ptrC, p.params->at(1));
		*p.state->l << mov(ptrA, p.params->at(0));
		*p.state->l << mov(doubleRel(ptrA, Offset()), doubleRel(ptrC, Offset()));
	}

	static void doubleAdd(InlineParams p) {
		if (p.result->needed()) {
			*p.state->l << fld(p.params->at(0));
			*p.state->l << fld(p.params->at(1));
			*p.state->l << faddp();
			*p.state->l << fstp(p.result->location(p.state).v);
			*p.state->l << fwait();
		}
	}

	static void doubleSub(InlineParams p) {
		if (p.result->needed()) {
			*p.state->l << fld(p.params->at(0));
			*p.state->l << fld(p.params->at(1));
			*p.state->l << fsubp();
			*p.state->l << fstp(p.result->location(p.state).v);
			*p.state->l << fwait();
		}
	}

	static void doubleMul(InlineParams p) {
		if (p.result->needed()) {
			*p.state->l << fld(p.params->at(0));
			*p.state->l << fld(p.params->at(1));
			*p.state->l << fmulp();
			*p.state->l << fstp(p.result->location(p.state).v);
			*p.state->l << fwait();
		}
	}

	static void doubleDiv(InlineParams p) {
		if (p.result->needed()) {
			*p.state->l << fld(p.params->at(0));
			*p.state->l << fld(p.params->at(1));
			*p.state->l << fdivp();
			*p.state->l << fstp(p.result->location(p.state).v);
			*p.state->l << fwait();
		}
	}

	static void doubleAssign(InlineParams p) {
		*p.state->l << mov(ptrA, p.params->at(0));
		*p.state->l << mov(doubleRel(ptrA, Offset()), p.params->at(1));
		if (p.result->needed())
			if (!p.result->suggest(p.state, p.params->at(0)))
				*p.state->l << mov(p.result->location(p.state).v, doubleRel(ptrA, Offset()));
	}

	static void doubleToInt(InlineParams p) {
		if (!p.result->needed())
			return;
		*p.state->l << fld(p.params->at(0));
		*p.state->l << fistp(p.result->location(p.state).v);
	}

	template <CondFlag f>
	static void doubleCmp(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << fld(p.params->at(1));
			*p.state->l << fld(p.params->at(0));
			*p.state->l << fcompp();
			*p.state->l << setCond(result, f);
		}
	}

	static Double doubleMin(Double a, Double b) {
		return min(a, b);
	}

	static Double doubleMax(Double a, Double b) {
		return max(a, b);
	}

	DoubleType::DoubleType(Str *name, GcType *type) : Type(name, typeValue | typeFinal, Size::sDouble, type, null) {}

	Bool DoubleType::loadAll() {
		//Array<Value> *r = new (this) Array<Value>(1, Value(this, true));
		Array<Value> *v = new (this) Array<Value>(1, Value(this, false));
		Array<Value> *rr = new (this) Array<Value>(2, Value(this, true));
		Array<Value> *vv = new (this) Array<Value>(2, Value(this, false));
		Array<Value> *rv = new (this) Array<Value>(2, Value(this, true));
		rv->at(1) = Value(this);

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &doubleCopyCtor)));
		add(inlinedFunction(engine, Value(this, true), S("="), rv, fnPtr(engine, &doubleAssign)));

		add(inlinedFunction(engine, Value(this), S("+"), vv, fnPtr(engine, &doubleAdd)));
		add(inlinedFunction(engine, Value(this), S("-"), vv, fnPtr(engine, &doubleSub)));
		add(inlinedFunction(engine, Value(this), S("*"), vv, fnPtr(engine, &doubleMul)));
		add(inlinedFunction(engine, Value(this), S("/"), vv, fnPtr(engine, &doubleDiv)));

		Value vBool = Value(StormInfo<Bool>::type(engine));
		add(inlinedFunction(engine, vBool, S(">"), vv, fnPtr(engine, &doubleCmp<ifFAbove>)));
		add(inlinedFunction(engine, vBool, S(">="), vv, fnPtr(engine, &doubleCmp<ifFAboveEqual>)));
		add(inlinedFunction(engine, vBool, S("<"), vv, fnPtr(engine, &doubleCmp<ifFBelow>)));
		add(inlinedFunction(engine, vBool, S("<="), vv, fnPtr(engine, &doubleCmp<ifFBelowEqual>)));
		add(inlinedFunction(engine, vBool, S("=="), vv, fnPtr(engine, &doubleCmp<ifEqual>)));
		add(inlinedFunction(engine, vBool, S("!="), vv, fnPtr(engine, &doubleCmp<ifNotEqual>)));

		Value vInt = Value(StormInfo<Int>::type(engine));
		add(inlinedFunction(engine, vInt, S("int"), v, fnPtr(engine, &doubleToInt)));
		Value vLong = Value(StormInfo<Long>::type(engine));
		add(inlinedFunction(engine, vLong, S("long"), v, fnPtr(engine, &doubleToInt))); // We can use the same function here!

		add(nativeFunction(engine, Value(this), S("min"), vv, address(&doubleMin)));
		add(nativeFunction(engine, Value(this), S("max"), vv, address(&doubleMax)));

		return Type::loadAll();
	}

}
