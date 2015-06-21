#include "stdafx.h"
#include "BSActual.h"
#include "BSAutocast.h"
#include "Lib/Maybe.h"

namespace storm {

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

		Auto<Expr> expr = castTo(expressions[id], param);
		assert(expr,
			L"Can not use " + ::toS(expressions[id]->result()) + L" as an actual value for parameter " + ::toS(param));

		Value exprResult = expr->result();
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
		exprs.insert(exprs.begin(), CREATE(DummyExpr, this, first));
	}

	void bs::BSNamePart::insert(Value first, Nat at) {
		params.insert(params.begin() + at, first);
		exprs.insert(exprs.begin() + at, CREATE(DummyExpr, this, first));
	}

	void bs::BSNamePart::alter(Nat at, Value to) {
		params[at] = to;
		exprs[at] = CREATE(DummyExpr, this, to);
	}

	// TODO: Consider using 'max' for match weights instead?
	Int bs::BSNamePart::matches(Par<Named> candidate) {
		const vector<Value> &c = candidate->params;
		if (c.size() != params.size())
			return -1;

		int distance = 0;

		for (nat i = 0; i < c.size(); i++) {
			const Value &formal = c[i];
			Par<Expr> actual = exprs[i];

			int penalty = castPenalty(actual, formal, candidate->flags);
			if (penalty >= 0)
				distance += penalty;
			else
				return -1;
		}

		return distance;
	}

	Name *bs::bsName(Par<Str> name, Par<Actual> params) {
		return CREATE(Name, params, steal(CREATE(BSNamePart, params, name, params)));
	}

	Name *bs::bsName(const String &name, Par<Actual> params) {
		return CREATE(Name, params, steal(CREATE(BSNamePart, params, name, params)));
	}

}
