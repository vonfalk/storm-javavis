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
			*p.state->l << mov(result, p.param(0));
			*p.state->l << band(result, p.param(1));
		}
	}

	static void boolOr(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << mov(result, p.param(0));
			*p.state->l << bor(result, p.param(1));
		}
	}

	static void boolEq(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << cmp(p.param(0), p.param(1));
			*p.state->l << setCond(result, ifEqual);
		}
	}

	static void boolNeq(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << cmp(p.param(0), p.param(1));
			*p.state->l << setCond(result, ifNotEqual);
		}
	}

	static void boolNot(InlineParams p) {
		if (p.result->needed()) {
			Operand result = p.result->location(p.state).v;
			*p.state->l << cmp(p.param(0), byteConst(0));
			*p.state->l << setCond(result, ifEqual);
		}
	}

	static void boolAssign(InlineParams p) {
		p.allocRegs(0);
		Reg dest = p.regParam(0);

		*p.state->l << mov(byteRel(dest, Offset()), p.param(1));
		if (p.result->needed()) {
			if (p.result->type().ref) {
				if (!p.result->suggest(p.state, p.originalParam(0)))
					*p.state->l << mov(p.result->location(p.state).v, dest);
			} else {
				if (!p.result->suggest(p.state, p.param(1)))
					*p.state->l << mov(p.result->location(p.state).v, p.param(1));
			}
		}
	}

	static void boolCopyCtor(InlineParams p) {
		p.allocRegs(0, 1);
		*p.state->l << mov(byteRel(p.regParam(0), Offset()), byteRel(p.regParam(1), Offset()));
	}


	BoolType::BoolType(Str *name, GcType *type) : Type(name, typeValue | typeFinal, Size::sByte, type, null) {}

	Bool BoolType::loadAll() {
		// Array<Value> *r = new (this) Array<Value>(1, Value(this, true));
		Array<Value> *v = new (this) Array<Value>(1, Value(this, false));
		Array<Value> *rr = new (this) Array<Value>(2, Value(this, true));
		Array<Value> *vv = new (this) Array<Value>(2, Value(this, false));
		Array<Value> *rv = new (this) Array<Value>(2, Value(this, true));
		rv->at(1) = Value(this);

		add(inlinedFunction(engine, Value(this), S("&"), vv, fnPtr(engine, &boolAnd))->makePure());
		add(inlinedFunction(engine, Value(this), S("|"), vv, fnPtr(engine, &boolOr))->makePure());
		add(inlinedFunction(engine, Value(this), S("=="), vv, fnPtr(engine, &boolEq))->makePure());
		add(inlinedFunction(engine, Value(this), S("!="), vv, fnPtr(engine, &boolNeq))->makePure());
		add(inlinedFunction(engine, Value(this), S("!"), v, fnPtr(engine, &boolNot))->makePure());

		add(inlinedFunction(engine, Value(), Type::CTOR, rr, fnPtr(engine, &boolCopyCtor))->makePure());
		add(inlinedFunction(engine, Value(this, true), S("="), rv, fnPtr(engine, &boolAssign))->makePure());

		return Type::loadAll();
	}

}
