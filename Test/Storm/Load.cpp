#include "stdafx.h"


BEGIN_TEST(Load, Storm) {
	Engine &e = gEngine();

	SimpleName *n = parseSimpleName(e, L"lang.bs.SExpr");
	e.scope().find(n);

} END_TEST
