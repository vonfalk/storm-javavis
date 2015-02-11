#include "stdafx.h"
#include "Simple.h"
#include "Lib/Array.h"

namespace storm {

	SExpr::SExpr() {}

	SScope::SScope() {}

	void SScope::expr(Par<SExpr> expr) {}

	SExpr *sOperator(Par<SExpr> lhs, Par<SExpr> rhs, Par<SStr> op) {
		return CREATE(SExpr, op);
	}

	SExpr *sVar(Par<SStr> op) {
		return CREATE(SExpr, op);
	}

	SExpr *sNr(Par<SStr> op) {
		return CREATE(SExpr, op);
	}

	ArrayP<Str> *convert(Par<ArrayP<SStr>> v) {
		ArrayP<Str> *result = CREATE(ArrayP<Str>, v);

		for (nat i = 0; i < v->count(); i++) {
			result->push(v->at(i)->v);
		}

		return result;
	}

	Str *consume(Par<ArrayP<Str>> v) {
		if (v->count() == 1)
			return v->at(0).ret();
		else
			return v->toS();
	}

}
