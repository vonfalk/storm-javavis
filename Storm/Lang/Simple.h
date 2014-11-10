#pragma once
#include "Std.h"

namespace storm {

	STORM_PKG(lang.simple);

	/**
	 * Functions used by the 'simple' syntax.
	 */

	class SExpr : public Object {
		STORM_CLASS;
	public:
		SExpr();
	};

	class SScope : public Object {
		STORM_CLASS;
	public:
		STORM_CTOR SScope();

		void STORM_FN expr(SExpr *expr);
	};


	SExpr *STORM_FN sOperator(SExpr *lhs, SExpr *rhs, Str *op);
	SExpr *STORM_FN sVar(Str *op);
	SExpr *STORM_FN sNr(Str *op);

}
