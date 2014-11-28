#include "stdafx.h"
#include "BSExpr.h"
#include "Lib/Int.h"

namespace storm {

	bs::Expr::Expr() {}

	Value bs::Expr::result() {
		return Value();
	}

	code::Listing bs::Expr::code(code::Variable var) {
		assert(var == code::Variable::invalid);
		return code::Listing();
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

	code::Listing bs::Constant::code(code::Variable var) {
		using namespace code;
		Listing r;

		if (var == code::Variable::invalid)
			return r;

		switch (cType) {
		case tInt:
			r << mov(var, intConst(value));
			break;
		default:
			TODO("Implement missing type");
			break;
		}

		return r;
	}

	bs::Constant *bs::intConstant(Auto<SStr> v) {
		return CREATE(Constant, v->engine(), v->v->v.toInt());
	}

}
