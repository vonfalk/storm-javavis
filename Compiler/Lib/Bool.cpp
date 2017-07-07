#include "stdafx.h"
#include "Bool.h"
#include "Function.h"

namespace storm {
	using namespace code;

	Type *createBool(Str *name, Size size, GcType *type) {
		return new (name) BoolType(name, type);
	}

	static void boolAnd(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << mov(result, p.params->at(0));
			*p.state->l << band(result, p.params->at(1));
		}
	}

	static void boolOr(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << mov(result, p.params->at(0));
			*p.state->l << bor(result, p.params->at(1));
		}
	}

	static void boolEq(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << cmp(p.params->at(0), p.params->at(1));
			*p.state->l << setCond(result, ifEqual);
		}
	}

	static void boolNeq(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << cmp(p.params->at(0), p.params->at(1));
			*p.state->l << setCond(result, ifNotEqual);
		}
	}

	static void boolNot(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << cmp(p.params->at(0), byteConst(0));
			*p.state->l << setCond(result, ifEqual);
		}
	}

	static void boolAssign(InlineParams p) {
		*p.state->l << mov(ptrA, p.params->at(0));
		*p.state->l << mov(byteRel(ptrA, Offset()), p.params->at(1));
		if (p.result->needed()) {
			if (p.result->type().ref) {
				if (!p.result->suggest(p.state, p.params->at(0)))
					*p.state->l << mov(p.result->location(p.state).v, ptrA);
			} else {
				if (!p.result->suggest(p.state, p.params->at(1)))
					*p.state->l << mov(p.result->location(p.state).v, p.params->at(1));
			}
		}
	}

	static void boolCopyCtor(InlineParams p) {
		*p.state->l << mov(ptrC, p.params->at(1));
		*p.state->l << mov(ptrA, p.params->at(0));
		*p.state->l << mov(byteRel(ptrA, Offset()), byteRel(ptrC, Offset()));
	}


	BoolType::BoolType(Str *name, GcType *type) : Type(name, typeValue | typeFinal, Size::sByte, type, null) {}

	Bool BoolType::loadAll() {
		// Array<Value> *r = new (this) Array<Value>(1, Value(this, true));
		Array<Value> *v = new (this) Array<Value>(1, Value(this, false));
		Array<Value> *rr = new (this) Array<Value>(2, Value(this, true));
		Array<Value> *vv = new (this) Array<Value>(2, Value(this, false));
		Array<Value> *rv = new (this) Array<Value>(2, Value(this, true));
		rv->at(1) = Value(this);

		add(inlinedFunction(engine, Value(this), S("&"), vv, fnPtr(engine, &boolAnd)));
		add(inlinedFunction(engine, Value(this), S("|"), vv, fnPtr(engine, &boolOr)));
		add(inlinedFunction(engine, Value(this), S("=="), vv, fnPtr(engine, &boolEq)));
		add(inlinedFunction(engine, Value(this), S("!="), vv, fnPtr(engine, &boolNeq)));
		add(inlinedFunction(engine, Value(this), S("!"), v, fnPtr(engine, &boolNot)));

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &boolCopyCtor)));
		add(inlinedFunction(engine, Value(this, true), S("="), rv, fnPtr(engine, &boolAssign)));

		return Type::loadAll();
	}

}
