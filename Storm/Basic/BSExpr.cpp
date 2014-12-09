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
		assert(!var.needed);
	}

	bs::Constant::Constant(Int v) : cType(tInt), value(v) {}

	void bs::Constant::output(wostream &to) const {
		switch (cType) {
		case tInt:
			to << value << L"(I)";
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
		default:
			TODO("Implement missing type");
			return Value();
		}
	}

	void bs::Constant::code(const GenState &s, GenResult &r) {
		using namespace code;

		if (!r.needed)
			return;

		switch (cType) {
		case tInt:
			s.to << mov(r.location(s, Value(intType(engine()))), intConst(value));
			break;
		default:
			TODO("Implement missing type");
			break;
		}
	}

	bs::Constant *bs::intConstant(Auto<SStr> v) {
		return CREATE(Constant, v->engine(), v->v->v.toInt());
	}

}
