#include "stdafx.h"
#include "Simple.h"

namespace storm {

	SExpr::SExpr() {}

	SScope::SScope() {}

	void SScope::expr(Auto<SExpr> expr) {}

	SExpr *sOperator(Auto<SExpr> lhs, Auto<SExpr> rhs, Auto<SStr> op) {
		return CREATE(SExpr, op);
	}

	SExpr *sVar(Auto<SStr> op) {
		return CREATE(SExpr, op);
	}

	SExpr *sNr(Auto<SStr> op) {
		return CREATE(SExpr, op);
	}

}
