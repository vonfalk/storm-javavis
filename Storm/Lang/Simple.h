#pragma once
#include "Std.h"

namespace storm {

	STORM_PKG(lang.simple);

	/**
	 * Functions used by the 'simple' syntax.
	 */

	STORM class SExpr : public Object {
	public:
	};

	STORM class SScope : public Object {
	public:
		STORM SScope(Type *t);

		void STORM_FN expr(SExpr *expr);
	};


	SExpr *STORM_FN sOperator(SExpr *lhs, SExpr *rhs, Str *op);
	SExpr *STORM_FN sVar(Str *op);
	SExpr *STORM_FN sNr(Str *op);

}
