#include "stdafx.h"
#include "BSActual.h"

namespace storm {

	bs::Actual::Actual() {}

	vector<Value> bs::Actual::values() {
		vector<Value> v(expressions.size());
		for (nat i = 0; i < expressions.size(); i++) {
			v[i] = expressions[i]->result();
		}
		return v;
	}

	void bs::Actual::add(Par<Expr> e) {
		expressions.push_back(e);
	}

	void bs::Actual::addFirst(Par<Expr> e) {
		expressions.insert(expressions.begin(), e);
	}


	/**
	 * Helper to compute an actual parameter. Takes care of ref/non-ref conversions.
	 * Returns the value into which the resulting parameter were placed.
	 */
	code::Value bs::Actual::code(nat id, GenState &s, const Value &param) {
		using namespace code;

		Expr *expr = expressions[id].borrow();
		Value exprResult = expr->result();
		assert(param.canStore(exprResult));

		if (param.ref && !exprResult.ref) {
			// We need to create a temporary variable and make a reference to it.
			Variable tmpV = variable(s, exprResult);
			GenResult gr(exprResult, tmpV);
			expr->code(s, gr);

			Variable tmpRef = variable(s, param);
			s.to << lea(tmpRef, ptrRel(tmpV));
			return tmpRef;
		} else {
			// 'expr' will handle the type we are giving it.
			GenResult gr(param, s.part);
			expr->code(s, gr);
			return gr.location(s);
		}
	}

	void bs::Actual::output(wostream &to) const {
		to << L"(";
		join(to, expressions, L", ");
		to << L")";
	}
}
