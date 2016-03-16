#include "stdafx.h"
#include "Test/Test.h"
#include "Fn.h"
#include "Storm/Syntax/Parser.h"

BEGIN_TEST(SyntaxTest) {

	CHECK_RUNS(runFn<int>(L"test.syntax.testSimple"));
	CHECK(runFn<Bool>(L"test.syntax.testSentence"));
	CHECK(runFn<Bool>(L"test.syntax.testMaybe"));
	CHECK(runFn<Bool>(L"test.syntax.testArray"));
	CHECK(runFn<Bool>(L"test.syntax.testCall"));
	CHECK(runFn<Bool>(L"test.syntax.testCallMaybe"));
	CHECK_EQ(runFn<Nat>(L"test.syntax.testRaw"), 2);
	CHECK_EQ(runFn<Nat>(L"test.syntax.testRawCall"), 2);
	CHECK(runFn<Bool>(L"test.syntax.testCapture"));
	CHECK(runFn<Bool>(L"test.syntax.testRawCapture"));
	CHECK_RUNS(runFn<int>(L"test.syntax.testParams"));
	CHECK(runFn<Bool>(L"test.syntax.testEmpty"));
	CHECK_EQ(runFn<Int>(L"test.syntax.testExpr"), 40);

} END_TEST

using namespace storm::syntax;

String parseStr(const String &root, const String &parse) {
	Package *pkg = gEngine->package(L"test.syntax");
	Auto<Parser> p = Parser::create(pkg, root);

	Auto<Url> empty = CREATE(Url, *gEngine);
	Auto<Str> s = CREATE(Str, *gEngine, parse);
	p->parse(s, empty);

	if (p->hasError())
		p->throwError();

	Auto<Object> r = p->transform<Object>();
	return ::toS(r);
}

BEGIN_TEST(ParseOrderTest) {
	CHECK_EQ(parseStr(L"Prio", L"a b"), L"ab");
	CHECK_EQ(parseStr(L"Prio", L"var b"), L"b");
	CHECK_EQ(parseStr(L"Prio", L"async b"), L"asyncb");

	CHECK_EQ(parseStr(L"Rec", L"a.b.c.d"), L"(((a)(b))(c))(d)");
	CHECK_EQ(parseStr(L"Rec", L"a,b,c,d"), L"(a)((b)((c)(d)))");
	CHECK_EQ(parseStr(L"Rec", L"a.b.c.d.e"), L"((((a)(b))(c))(d))(e)");
	CHECK_EQ(parseStr(L"Rec", L"a,b,c,d,e"), L"(a)((b)((c)((d)(e))))");

	CHECK_EQ(parseStr(L"Rec3", L"a.b.c.d.e.f.g"), L"(((a)(b)(c))(d)(e))(f)(g)");
	CHECK_EQ(parseStr(L"Rec3", L"a,b,c,d,e,f,g"), L"(a)(b)((c)(d)((e)(f)(g)))");
} END_TEST
