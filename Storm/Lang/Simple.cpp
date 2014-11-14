#include "stdafx.h"
#include "Simple.h"

namespace storm {

	SExpr::SExpr() : Object() {}

	SScope::SScope() : Object() {}

	void SScope::expr(Auto<SExpr> expr) {}

	SExpr *sOperator(Auto<SExpr> lhs, Auto<SExpr> rhs, Auto<Str> op) {
		return CREATE(SExpr, op);
	}

	SExpr *sVar(Auto<Str> op) {
		return CREATE(SExpr, op);
	}

	SExpr *sNr(Auto<Str> op) {
		return CREATE(SExpr, op);
	}

}
