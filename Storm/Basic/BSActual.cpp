#include "stdafx.h"
#include "BSActual.h"
#include "Lib/Maybe.h"

namespace storm {

	// Modify the formal parameter to match 'actual' if neccessary.
	// These modifications are always 100% binary compatible. This will probably be extended
	// in the future to support calling constructors in between.
	static bool autoCast(Value &formal, const Value &actual) {
		MaybeType *formalT = as<MaybeType>(formal.type);
		MaybeType *actualT = as<MaybeType>(actual.type);

		if (formalT != null && actualT == null) {
			// Preserve the 'ref' parameter!
			formal.type = formalT->param.type;
			return true;
		}

		return false;
	}

	bool bs::callable(const Value &formal, const Value &actual, Par<Expr> src) {
		return formal.canStore(actual) || src->castable(formal);
	}

	bs::Actual::Actual() {}

	bs::Actual::Actual(Par<Expr> expr) {
		add(expr);
	}

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
	code::Value bs::Actual::code(nat id, Par<CodeGen> s, Value param) {
		using namespace code;

		Expr *expr = expressions[id].borrow();
		Value exprResult = expr->result();

		if (!callable(param, exprResult, expr))
			autoCast(param, exprResult);

		assert(callable(param, exprResult, expr),
			L"Can not use " + ::toS(exprResult) + L" as an actual value for parameter " + ::toS(param));

		if (param.ref && !exprResult.ref) {
			// We need to create a temporary variable and make a reference to it.
			VarInfo tmpV = variable(s, exprResult);
			Auto<CodeResult> gr = CREATE(CodeResult, this, exprResult, tmpV);
			expr->code(s, gr);

			VarInfo tmpRef = variable(s, param);
			s->to << lea(tmpRef.var(), ptrRel(tmpV.var()));
			tmpRef.created(s);
			return tmpRef.var();
		} else {
			// 'expr' will handle the type we are giving it.
			Auto<CodeResult> gr = CREATE(CodeResult, this, param, s->block);
			expr->code(s, gr);
			return gr->location(s).var();
		}
	}

	void bs::Actual::output(wostream &to) const {
		to << L"(";
		join(to, expressions, L", ");
		to << L")";
	}


	bs::BSNamePart::BSNamePart(Par<Str> name, Par<Actual> params) :
		NamePart(name->v, params->values()), exprs(params->expressions) {}

	bs::BSNamePart::BSNamePart(const String &name, Par<Actual> params) :
		NamePart(name, params->values()), exprs(params->expressions) {}

	void bs::BSNamePart::insert(Value first) {
		params.insert(params.begin(), first);
		exprs.insert(exprs.begin(), null);
	}

	Int bs::BSNamePart::matches(Par<Named> candidate) {
		const vector<Value> &c = candidate->params;
		if (c.size() != params.size())
			return -1;

		int distance = 0;

		for (nat i = 0; i < c.size(); i++) {
			Value formal = c[i];
			const Value &actual = params[i];

			if (autoCast(formal, actual))
				distance += 100; // Penalty

			if (formal.matches(actual, candidate->matchFlags)) {
				if (actual.type)
					distance += actual.type->distanceFrom(formal.type);
			} else if (exprs[i] && exprs[i]->castable(actual)) {
				// Penalty for this casting...
				distance += 100;
			} else {
				return -1;
			}
		}

		return distance;
	}
}
