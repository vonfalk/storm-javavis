#include "stdafx.h"
#include "Simple.h"
#include "Lib/Array.h"

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

	Str *consume(Par<Object> obj) {
		if (as<ArrayBase>(obj.borrow())) {
			// Probably an Array<Object>... We can not put those in function
			// declarations yet...
			Array<Auto<Object>> *z = (Array<Auto<Object>> *)obj.borrow();
			if (z->count() == 0)
				return CREATE(Str, z, L"");

			if (SStr *c = as<SStr>(z->at(0).borrow()))
				return c->v.ret();

			return z->at(0)->toS();
		}

		return obj->toS();
	}

}
