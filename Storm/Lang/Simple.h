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

		void STORM_FN expr(Auto<SExpr> expr);
	};


	SExpr *STORM_FN sOperator(Auto<SExpr> lhs, Auto<SExpr> rhs, Auto<Str> op);
	SExpr *STORM_FN sVar(Auto<Str> op);
	SExpr *STORM_FN sNr(Auto<Str> op);

}
