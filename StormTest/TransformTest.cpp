#include "stdafx.h"
#include "Test/Test.h"

#include "Storm/Engine.h"
#include "Storm/Exception.h"
#include "Storm/Parser.h"

using namespace storm;

bool tfm(SyntaxSet &set, const String &root, const String &str) {
	Parser p(set, str);
	if (!p.parse(root))
		return false;
	SyntaxNode *t = p.tree();
	if (!t)
		return false;

	PLN(*t);
	Object *o = transform(*t);

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
	CHECK(tfm(set, L"Root", L"a + b"));

} END_TEST
