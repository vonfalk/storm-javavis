#include "stdafx.h"
#include "Test/Test.h"

#include "Storm/Engine.h"
#include "Storm/Exception.h"
#include "Storm/Parser.h"
#include "Storm/SyntaxTransform.h"

using namespace storm;

bool tfm(Engine &e, Par<SyntaxSet> set, const String &root, const String &str, Auto<Object> eqTo) {
	Auto<Url> empty = CREATE(Url, e);
	Auto<Str> s = CREATE(Str, e, str);
	Auto<Parser> p = CREATE(Parser, e, set, s, empty);
	if (p->parse(root) == Parser::NO_MATCH) {
		PLN("Parse failure.");
		return false;
	}

	SyntaxNode *t = p->tree();
	if (!t) {
		PLN("No tree.");
		return false;
	}
	// PLN(*t);

	try {
		Auto<Object> o = transform(e, *set.borrow(), *t);
		bool result = true;
		if (eqTo) {
			if (SStr *oo = as<SStr>(o.borrow())) {
				result = false;
				if (SStr *et = as<SStr>(eqTo.borrow())) {
					result = oo->v->equals(et->v);
				}
			} else {
				result = eqTo->equals(o);
			}
		}

		if (!result) {
			PLN("Got: " << *o << L", expected: " << *eqTo);
		}

		delete t;
		return result;
	} catch (...) {
		delete t;
		throw;
	}
}

BEGIN_TEST(TransformTest) {

	Engine &engine = *gEngine;

	Package *simple = engine.package(L"lang.simple");
	Auto<SyntaxSet> set = CREATE(SyntaxSet, engine);
	set->add(simple);

	CHECK(tfm(engine, set, L"Rep1Root", L"{ a + b; }", null));
	CHECK(tfm(engine, set, L"CaptureRoot", L"{ a + b; }", CREATE(SStr, engine, L"{ a + b; }")));
	CHECK(tfm(engine, set, L"Capture2Root", L"-{ a + b; }-", CREATE(SStr, engine, L"{ a + b; }")));
	CHECK(tfm(engine, set, L"CaptureEmpty", L"- -", CREATE(SStr, engine, L" ")));
	CHECK(tfm(engine, set, L"CaptureEmpty", L"--", CREATE(SStr, engine, L"")));

	CHECK(tfm(engine, set, L"EmptyVal", L"()", CREATE(Str, engine, L"[]")));
	CHECK(tfm(engine, set, L"EmptyVal", L"(abcabc)", CREATE(Str, engine, L"abcabc")));

} END_TEST
