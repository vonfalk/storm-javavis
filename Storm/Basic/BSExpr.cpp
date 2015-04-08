#include "stdafx.h"
#include "BSExpr.h"
#include "BSBlock.h"
#include "Lib/Int.h"
#include "Exception.h"
#include "function.h"

namespace storm {

	bs::Expr::Expr() {}

	Value bs::Expr::result() {
		return Value();
	}

	void bs::Expr::code(const GenState &to, GenResult &var) {
		assert(!var.needed());
	}

	bs::Constant::Constant(Int v) : cType(tInt), intValue(v) {}

	bs::Constant::Constant(Str *v) : cType(tStr), strValue(capture(v)) {}

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

	void bs::Constant::code(const GenState &s, GenResult &r) {
		using namespace code;

		if (!r.needed())
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

	void bs::Constant::strCode(const GenState &s, GenResult &r) {
		using namespace code;

		Engine &e = engine();

		Label data = s.to.label();
		s.to << fnParam(Str::stormType(e)->typeRef);
		s.to << fnParam(data);
		s.to << fnCall(e.fnRefs.createStrFn, Size::sPtr);
		VarInfo to = r.location(s);
		s.to << mov(to.var(), ptrA);
		to.created(s);

		s.data.add(data, memberFn(this, &Constant::strData), this);
	}

	void bs::Constant::intCode(const GenState &s, GenResult &r) {
		using namespace code;

		VarInfo to = r.location(s);
		s.to << mov(to.var(), intConst(intValue));
		to.created(s);
	}

	void bs::Constant::boolCode(const GenState &s, GenResult &r) {
		using namespace code;

		VarInfo to = r.location(s);
		s.to << mov(to.var(), byteConst(boolValue ? 1 : 0));
		to.created(s);
	}

	void bs::Constant::strData(code::Listing &to) {
		using namespace code;

		// Generate the string in memory!
		const String &v = strValue->v;
		for (nat i = 0; i < v.size(); i++) {
			to << dat(byteConst(v[i] & 0xFF));
			to << dat(byteConst(v[i] >> 8));
		}
		to << dat(byteConst(0)) << dat(byteConst(0));
	}

	bs::Constant *bs::intConstant(Par<SStr> v) {
		return CREATE(Constant, v->engine(), v->v->v.toInt());
	}

	bs::Constant *bs::strConstant(Par<SStr> v) {
		return CREATE(Constant, v->engine(), v->v.borrow());
	}

}
