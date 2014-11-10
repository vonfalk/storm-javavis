#include "stdafx.h"
#include "Test/Test.h"

#include "Storm/Engine.h"
#include "Storm/Exception.h"
#include "Storm/Parser.h"
#include "Storm/SyntaxTransform.h"

using namespace storm;

bool tfm(Engine &e, SyntaxSet &set, const String &root, const String &str, Object *eqTo) {
	Parser p(set, str);
	if (!p.parse(root))
		return false;
	SyntaxNode *t = p.tree();
	if (!t)
		return false;
	//PLN(*t);

	Object *o = null;
	try {
		o = transform(e, set, *t);
	} catch (...) {
		delete t;
		eqTo->release();
		throw;
	}

	bool result = true;
	if (eqTo)
		result = eqTo->equals(o);

	// Null-safe!
	o->release();
	eqTo->release();
	delete t;

	return result;
}

BEGIN_TEST(TransformTest) {

	Path root = Path::executable() + Path(L"../root/");
	Engine engine(root);

	Package *simple = engine.package(Name(L"lang.simple"));
	SyntaxSet set;
	set.add(*simple);
	CHECK(tfm(engine, set, L"Rep1Root", L"{ a + b; }", null));

	CHECK(tfm(engine, set, L"CaptureRoot", L"{ a + b; }", CREATE(Str, engine, L"{ a + b; }")));
	CHECK(tfm(engine, set, L"Capture2Root", L"-{ a + b; }-", CREATE(Str, engine, L"{ a + b; }")));

} END_TEST
