#include "stdafx.h"
#include "Test/Test.h"

#include "Storm/Engine.h"
#include "Storm/Exception.h"
#include "Storm/Parser.h"
#include "Storm/SyntaxTransform.h"

using namespace storm;

bool tfm(Engine &e, SyntaxSet &set, const String &root, const String &str, Auto<Object> eqTo) {
	Parser p(set, str, Path());
	if (!p.parse(root)) {
		eqTo->release();
		return false;
	}

	SyntaxNode *t = p.tree();
	if (!t) {
		return false;
	}
	// PLN(*t);

	try {
		Auto<Object> o = transform(e, set, *t);
		bool result = true;
		if (eqTo) {
			result = eqTo->equals(o);
			if (!result) {
				PLN("Got: " << *o << L", expected: " << *eqTo);
			}
		}

		delete t;
		return result;
	} catch (...) {
		delete t;
		throw;
	}
}

BEGIN_TEST(TransformTest) {

	Path root = Path::executable() + Path(L"../root/");
	Engine engine(root);

	Package *simple = engine.package(Name(L"lang.simple"));
	SyntaxSet set;
	set.add(*simple);
	CHECK(tfm(engine, set, L"Rep1Root", L"{ a + b; }", null));

	CHECK(tfm(engine, set, L"CaptureRoot", L"{ a + b; }", CREATE(SStr, engine, L"{ a + b; }")));
	CHECK(tfm(engine, set, L"Capture2Root", L"-{ a + b; }-", CREATE(SStr, engine, L"{ a + b; }")));

} END_TEST
