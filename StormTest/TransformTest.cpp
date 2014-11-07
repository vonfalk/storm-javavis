#include "stdafx.h"
#include "Test/Test.h"

#include "Storm/Engine.h"
#include "Storm/Exception.h"
#include "Storm/Parser.h"
#include "Storm/SyntaxTransform.h"

using namespace storm;

bool tfm(Engine &e, SyntaxSet &set, const String &root, const String &str) {
	Parser p(set, str);
	if (!p.parse(root))
		return false;
	SyntaxNode *t = p.tree();
	if (!t)
		return false;
	PLN(*t);

	Object *o = null;
	try {
		o = transform(e, set, *t);
	} catch (...) {
		delete t;
		throw;
	}

	// Null-safe!
	o->release();
	delete t;
	return true;
}

BEGIN_TEST(TransformTest) {

	Path root = Path::executable() + Path(L"../root/");
	Engine engine(root);

	Package *simple = engine.package(Name(L"lang.simple"));
	SyntaxSet set;
	set.add(*simple);
	CHECK(tfm(engine, set, L"Rep1Root", L"{ a + b; }"));

} END_TEST
