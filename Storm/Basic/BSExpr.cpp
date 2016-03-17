#include "stdafx.h"
#include "BSExpr.h"
#include "BSBlock.h"
#include "Lib/Int.h"
#include "Exception.h"
#include "Function.h"
#include "Engine.h"

namespace storm {

	bs::Expr::Expr(SrcPos pos) : pos(pos) {}

	ExprResult bs::Expr::result() {
		return ExprResult();
	}

	void bs::Expr::code(Par<CodeGen> to, Par<CodeResult> var) {
		assert(!var->needed());
	}

	Int bs::Expr::castPenalty(Value to) {
		return -1;
	}

	bs::Constant::Constant(SrcPos pos, Int v) : Expr(pos), cType(tInt), intValue(v) {}

	bs::Constant::Constant(SrcPos pos, Long v) : Expr(pos), cType(tInt), intValue(v) {}

	bs::Constant::Constant(SrcPos pos, Float v) : Expr(pos), cType(tFloat), floatValue(v) {}

	bs::Constant::Constant(SrcPos pos, double v) : Expr(pos), cType(tFloat), floatValue(v) {}

	bs::Constant::Constant(SrcPos pos, Str *v) : Expr(pos), cType(tStr), strValue(v->v) {}

	bs::Constant::Constant(SrcPos pos, const String &s) : Expr(pos), cType(tStr), strValue(s) {}

	bs::Constant::Constant(SrcPos pos, Bool v) : Expr(pos), cType(tBool), boolValue(v) {}

	void bs::Constant::output(wostream &to) const {
		switch (cType) {
		case tInt:
			to << intValue << L"i";
			break;
		case tFloat:
			to << floatValue << L"f";
			break;
		case tStr:
			to << L"\"" << strValue << L"\"";
			break;
		case tBool:
			to << (boolValue ? L"true" : L"false");
			break;
		default:
			to << L"UNKNOWN";
			break;
		}
	}

	ExprResult bs::Constant::result() {
		switch (cType) {
		case tInt:
			return Value(intType(engine()));
		case tFloat:
			return Value(floatType(engine()));
		case tStr:
			return Value(Str::stormType(engine()));
		case tBool:
			return Value(boolType(engine()));
		default:
			TODO("Implement missing type");
			return Value();
		}
	}

	Int bs::Constant::castPenalty(Value to) {
		if (cType != tInt)
			return -1;

		if (to.ref)
			return -1;

		// Prefer bigger types if multiple are possible.
		Engine &e = engine();
		if (to.type == longType(e))
			return 1;
		if (to.type == wordType(e))
			return 1;
		if (to.type == intType(e) && (abs(intValue) & 0x7FFFFFFF) == abs(intValue))
			return 2;
		if (to.type == natType(e) && (intValue & 0xFFFFFFFF) == intValue)
			return 2;
		if (to.type == byteType(e) && (intValue & 0xFF) == intValue)
			return 3;
		if (to.type == floatType(e) && (abs(intValue) & 0xFFFF) == abs(intValue))
			// We allow up to 16 bits to automatically cast.
			return 3;

		return -1;
	}

	void bs::Constant::code(Par<CodeGen> s, Par<CodeResult> r) {
		using namespace code;

		if (!r->needed())
			return;

		switch (cType) {
		case tInt:
			intCode(s, r);
			break;
		case tFloat:
			floatCode(s, r);
			break;
		case tStr:
			strCode(s, r);
			break;
		case tBool:
			boolCode(s, r);
			break;
		default:
			TODO("Implement missing type");
			break;
		}
	}

	void bs::Constant::strCode(Par<CodeGen> s, Par<CodeResult> r) {
		using namespace code;

		Engine &e = engine();

		Label data = s->to.label();
		s->to << fnParam(Str::stormType(e)->typeRef);
		s->to << fnParam(data);
		s->to << fnCall(e.fnRefs.createStrFn, retPtr());
		VarInfo to = r->location(s);
		s->to << mov(to.var(), ptrA);
		to.created(s);

		s->data->add(data, memberFn(this, &Constant::strData), this);
	}

	void bs::Constant::intCode(Par<CodeGen> s, Par<CodeResult> r) {
		using namespace code;

		VarInfo to = r->location(s);

		Type *t = r->type().type;
		Engine &e = engine();
		if (t == intType(e))
			s->to << mov(to.var(), intConst(int(intValue)));
		else if (t == natType(e))
			s->to << mov(to.var(), natConst(nat(intValue)));
		else if (t == byteType(e))
			s->to << mov(to.var(), byteConst(byte(intValue)));
		else if (t == floatType(e))
			s->to << mov(to.var(), floatConst(float(intValue)));
		else if (t == longType(e))
			s->to << mov(to.var(), longConst(Long(intValue)));
		else if (t == wordType(e))
			s->to << mov(to.var(), wordConst(Word(intValue)));
		else
			assert(false, L"Unknown type for an integer constant.");

		to.created(s);
	}

	void bs::Constant::floatCode(Par<CodeGen> s, Par<CodeResult> r) {
		using namespace code;

		VarInfo to = r->location(s);
		Type *t = r->type().type;
		Engine &e = engine();
		if (t != floatType(e))
			assert(false, L"Unknown type for a float constant.");

		s->to << mov(to.var(), floatConst(float(floatValue)));

		to.created(s);
	}

	void bs::Constant::boolCode(Par<CodeGen> s, Par<CodeResult> r) {
		using namespace code;

		VarInfo to = r->location(s);
		s->to << mov(to.var(), byteConst(boolValue ? 1 : 0));
		to.created(s);
	}

	void bs::Constant::strData(code::Listing &to) {
		using namespace code;

		// Generate the string in memory!
		// TODO: Write int16:s directly instead...
		const String &v = strValue;
		for (nat i = 0; i < v.size(); i++) {
			to << dat(byteConst(v[i] & 0xFF));
			to << dat(byteConst(v[i] >> 8));
		}
		to << dat(byteConst(0)) << dat(byteConst(0));
	}

	bs::Constant *bs::intConstant(SrcPos pos, Par<Str> v) {
		return CREATE(Constant, v->engine(), pos, v->v.toInt64());
	}

	bs::Constant *bs::floatConstant(SrcPos pos, Par<Str> v) {
		return CREATE(Constant, v->engine(), pos, v->v.toDouble());
	}

	bs::Constant *bs::strConstant(SrcPos pos, Par<Str> v) {
		return CREATE(Constant, v->engine(), pos, v->v.unescape());
	}

	bs::Constant *bs::rawStrConstant(SrcPos pos, Par<Str> v) {
		return CREATE(Constant, v->engine(), pos, v.borrow());
	}

	bs::Constant *bs::trueConstant(EnginePtr e, SrcPos pos) {
		return CREATE(Constant, e.v, pos, true);
	}

	bs::Constant *bs::falseConstant(EnginePtr e, SrcPos pos) {
		return CREATE(Constant, e.v, pos, false);
	}

	/**
	 * Dummy expression.
	 */

	bs::DummyExpr::DummyExpr(SrcPos pos, Value type) : Expr(pos), type(type) {}

	ExprResult bs::DummyExpr::result() {
		return type;
	}

	void bs::DummyExpr::code(Par<CodeGen> s, Par<CodeResult> r) {
		throw InternalError(L"Tried to generate code from a DummyExpr!");
	}

}
