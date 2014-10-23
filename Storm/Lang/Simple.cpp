#include "stdafx.h"
#include "Simple.h"

namespace storm {

	SScope::SScope(Type *t) : Object(t) {}

	void SScope::expr(SExpr *expr) {}

	SExpr *sOperator(SExpr *lhs, SExpr *rhs, Str *op) { return null; }

	SExpr *sVar(Str *op) { return null; }

	SExpr *sNr(Str *op) { return null; }

}
