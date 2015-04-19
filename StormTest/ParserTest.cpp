#include "stdafx.h"
#include "Test/Test.h"

#include "Storm/Engine.h"
#include "Storm/Exception.h"
#include "Storm/Parser.h"

using namespace storm;

nat parse(Par<SyntaxSet> set, const String &root, const String &str) {
	Auto<Url> empty = CREATE(Url, *gEngine);
	Auto<Str> s = CREATE(Str, *gEngine, str);
	Auto<Parser> p = CREATE(Parser, *gEngine, set, s, empty);
	nat r = p->parse(root);
	if (p->hasError())
		return r;

	SyntaxNode *t = p->tree();
	if (t) {
		// PLN(*t);
		delete t;
	}
	return r;
}

BEGIN_TEST(ParserTest) {

	Engine &engine = *gEngine;

	// Try to get the syntax for the 'sto' file format.
	Package *coreSto = engine.package(L"lang.sto");
	coreSto->syntax();

	// Try parsing a file with the simple syntax.
	Package *simple = engine.package(L"lang.simple");
	simple->syntax();

	Auto<SyntaxSet> set = CREATE(SyntaxSet, engine);
	set->add(simple);

	CHECK_EQ(parse(set, L"Root", L"a + b"), 5);
	CHECK_EQ(parse(set, L"Root", L"a + b-"), 5);
	CHECK_EQ(parse(set, L"Root", L"a + "), 1);
	CHECK_EQ(parse(set, L"Root", L"banana + rock + 33"), 18);

	CHECK_EQ(parse(set, L"Rep1Root", L"{ a; b; 1 + 2;}"), 15);
	CHECK_EQ(parse(set, L"Rep2Root", L"{ a; b; 1 + 2;}"), 15);
	CHECK_EQ(parse(set, L"Rep2Root", L"{}"), 2);
	CHECK_EQ(parse(set, L"Rep3Root", L"{ a; }"), 6);
	CHECK_EQ(parse(set, L"Rep3Root", L"{}"), 2);
	CHECK_EQ(parse(set, L"Rep3Root", L"{ a; b; }"), Parser::NO_MATCH);
	CHECK_EQ(parse(set, L"Rep4Root", L"a.b"), 3);
	CHECK_EQ(parse(set, L"Rep4Root", L"b"), 1);

	CHECK_EQ(parse(set, L"Empty", L"()"), 2);
	CHECK_EQ(parse(set, L"Empty2", L"()"), 2);

} END_TEST

String parseStr(Par<SyntaxSet> set, const String &root, const String &str) {
	Auto<Url> empty = CREATE(Url, *gEngine);
	Auto<Str> s = CREATE(Str, *gEngine, str);
	Auto<Parser> p = CREATE(Parser, *gEngine, set, s, empty);
	p->parse(root);
	if (p->hasError())
		throw p->error();

	Auto<Object> o = p->transform();
	return ::toS(o);
}

BEGIN_TEST(ParseOrderTest) {
	Engine &e = *gEngine;

	Package *simple = e.package(L"test.syntax");
	simple->syntax();

	Auto<SyntaxSet> set = CREATE(SyntaxSet, e);
	set->add(simple);

	CHECK_EQ(parseStr(set, L"Prio", L"a b"), L"ab");
	CHECK_EQ(parseStr(set, L"Prio", L"var b"), L"b");
	CHECK_EQ(parseStr(set, L"Prio", L"async b"), L"asyncb");

	CHECK_EQ(parseStr(set, L"Rec", L"a.b.c.d"), L"(((a)(b))(c))(d)");
	CHECK_EQ(parseStr(set, L"Rec", L"a,b,c,d"), L"(a)((b)((c)(d)))");
	CHECK_EQ(parseStr(set, L"Rec", L"a.b.c.d.e"), L"((((a)(b))(c))(d))(e)");
	CHECK_EQ(parseStr(set, L"Rec", L"a,b,c,d,e"), L"(a)((b)((c)((d)(e))))");

	CHECK_EQ(parseStr(set, L"Rec3", L"a.b.c.d.e.f.g"), L"(((a)(b)(c))(d)(e))(f)(g)");
	CHECK_EQ(parseStr(set, L"Rec3", L"a,b,c,d,e,f,g"), L"(a)(b)((c)(d)((e)(f)(g)))");
} END_TEST
