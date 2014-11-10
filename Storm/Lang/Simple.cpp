#include "stdafx.h"
#include "Simple.h"

namespace storm {

	SExpr::SExpr() : Object() {}

	SScope::SScope() : Object() {}

	void SScope::expr(SExpr *expr) {}

	SExpr *sOperator(SExpr *lhs, SExpr *rhs, Str *op) {
		return CREATE(SExpr, op);
	}

	SExpr *sVar(Str *op) {
		return CREATE(SExpr, op);
	}

	SExpr *sNr(Str *op) {
		return CREATE(SExpr, op);
	}

}
