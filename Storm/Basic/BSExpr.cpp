#include "stdafx.h"
#include "BSExpr.h"
#include "BSBlock.h"
#include "Lib/Int.h"
#include "Exception.h"
#include "Function.h"
#include "Engine.h"

namespace storm {

	bs::Expr::Expr() {}

	Value bs::Expr::result() {
		return Value();
	}

	void bs::Expr::code(Par<CodeGen> to, Par<CodeResult> var) {
		assert(!var->needed());
	}

	Bool bs::Expr::castable(Value to) {
		return false;
	}

	bs::Constant::Constant(Int v) : cType(tInt), intValue(v) {}

	bs::Constant::Constant(int64 v) : cType(tInt), intValue(v) {}

	bs::Constant::Constant(Str *v) : cType(tStr), strValue(v->v.unescape()) {}

	bs::Constant::Constant(Bool v) : cType(tBool), boolValue(v) {}

	void bs::Constant::output(wostream &to) const {
		switch (cType) {
		case tInt:
			to << intValue << L"i";
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

	Value bs::Constant::result() {
		switch (cType) {
		case tInt:
			return Value(intType(engine()));
		case tStr:
			return Value(Str::stormType(engine()));
		case tBool:
			return Value(boolType(engine()));
		default:
			TODO("Implement missing type");
			return Value();
		}
	}

	Bool bs::Constant::castable(Value to) {
		if (cType != tInt)
			return false;

		if (to.ref)
			return false;

		Engine &e = engine();
		if (to.type == intType(e))
			return (intValue & 0x7FFFFFFF) == intValue;
		if (to.type == natType(e))
			return (intValue & 0xFFFFFFFF) == intValue;
		if (to.type == byteType(e))
			return (intValue & 0xFF) == intValue;

		return false;
	}

	void bs::Constant::code(Par<CodeGen> s, Par<CodeResult> r) {
		using namespace code;

		if (!r->needed())
			return;

		switch (cType) {
		case tInt:
			intCode(s, r);
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
		s->to << fnCall(e.fnRefs.createStrFn, Size::sPtr);
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
		else
			assert(false, L"Unknown type for an integer constant.");

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

	bs::Constant *bs::intConstant(Par<SStr> v) {
		return CREATE(Constant, v->engine(), v->v->v.toInt64());
	}

	bs::Constant *bs::strConstant(Par<SStr> v) {
		return CREATE(Constant, v->engine(), v->v.borrow());
	}

}
