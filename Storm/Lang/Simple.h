#pragma once
#include "Std.h"
#include "SyntaxObject.h"
#include "Lib/Array.h"

namespace storm {

	STORM_PKG(lang.simple);

	/**
	 * Functions used by the 'simple' syntax.
	 */

	class SExpr : public SObject {
		STORM_CLASS;
	public:
		SExpr();
	};

	class SScope : public SObject {
		STORM_CLASS;
	public:
		STORM_CTOR SScope();

		void STORM_FN expr(Par<SExpr> expr);
	};


	SExpr *STORM_FN sOperator(Par<SExpr> lhs, Par<SExpr> rhs, Par<SStr> op);
	SExpr *STORM_FN sVar(Par<SStr> op);
	SExpr *STORM_FN sNr(Par<SStr> op);

	ArrayP<Str> *STORM_FN convert(Par<ArrayP<SStr>> v);
	Str *STORM_FN consume(Par<ArrayP<Str>> v);
}
