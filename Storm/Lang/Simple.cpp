#include "stdafx.h"
#include "Simple.h"

namespace storm {

	SExpr::SExpr(Type *t) : Object(t) {}

	SScope::SScope(Type *t) : Object(t) {}

	void SScope::expr(SExpr *expr) {}

	SExpr *sOperator(SExpr *lhs, SExpr *rhs, Str *op) {
		return new SExpr(SExpr::type(op));
	}

	SExpr *sVar(Str *op) {
		return new SExpr(SExpr::type(op));
	}

	SExpr *sNr(Str *op) {
		return new SExpr(SExpr::type(op));
	}

}
