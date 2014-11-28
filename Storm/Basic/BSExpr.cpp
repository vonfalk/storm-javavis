#include "stdafx.h"
#include "BSExpr.h"

namespace storm {

	bs::Expr::Expr() {}

	bs::Constant::Constant(Auto<SStr> val) {
		PLN("Constant: " << val);
	}
}
