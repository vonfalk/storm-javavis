#include "stdafx.h"
#include "Simple.h"

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

}
