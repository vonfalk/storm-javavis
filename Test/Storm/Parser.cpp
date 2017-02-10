#include "stdafx.h"
#include "Compiler/Syntax/Parser.h"
#include "Compiler/Package.h"
#include "Compiler/Syntax/Earley/Parser.h"
#include "Compiler/Syntax/GLR/Parser.h"
#include "Fn.h"

using syntax::Parser;

BEGIN_TEST_(ParserTest, Storm) {
	Engine &e = gEngine();

	Package *pkg = as<Package>(e.scope().find(parseSimpleName(e, L"test.grammar")));
	VERIFY(pkg);

	{
		// GLR parser.
		Parser *p = Parser::create(pkg, L"Sentence", new (e) storm::syntax::glr::Parser());
		Str *s = new (e) Str(L"the cat runs");
		CHECK(p->parse(s, new (e) Url()));
		CHECK(!p->hasError());
		CHECK(p->hasTree());
		CHECK(p->matchEnd() == s->end());

		// syntax::Node *tree = p->tree();
		// Str *r = syntax::transformNode<Str>(tree);
		// CHECK_EQ(::toS(r), L"cat");

		// CHECK(p->parse(new (e) Str(L"the cat runs!"), new (e) Url()));
		// CHECK(p->hasError());
		// CHECK(p->hasTree());
		// CHECK_EQ(p->matchEnd().v(), Char('!'));
	}

	{
		// Earley parser.
		Parser *p = Parser::create(pkg, L"Sentence", new (e) storm::syntax::earley::Parser());
		Str *s = new (e) Str(L"the cat runs");
		CHECK(p->parse(s, new (e) Url()));
		CHECK(!p->hasError());
		CHECK(p->hasTree());
		CHECK(p->matchEnd() == s->end());

		syntax::Node *tree = p->tree();
		Str *r = syntax::transformNode<Str>(tree);
		CHECK_EQ(::toS(r), L"cat");

		CHECK(p->parse(new (e) Str(L"the cat runs!"), new (e) Url()));
		CHECK(p->hasError());
		CHECK(p->hasTree());
		CHECK_EQ(p->matchEnd().v(), Char('!'));
	}

} END_TEST


/**
 * The tests below execute code in BS to get a relevant result.
 */

BEGIN_TEST(SyntaxTest, BS) {
	CHECK_RUNS(runFn<void>(L"test.syntax.testSimple"));
	CHECK(runFn<Bool>(L"test.syntax.testSentence"));
	CHECK(runFn<Bool>(L"test.syntax.testMaybe"));
	CHECK(runFn<Bool>(L"test.syntax.testArray"));
	CHECK(runFn<Bool>(L"test.syntax.testCall"));
	CHECK(runFn<Bool>(L"test.syntax.testCallMaybe"));
	CHECK_EQ(runFn<Nat>(L"test.syntax.testRaw"), 2);
	CHECK_EQ(runFn<Nat>(L"test.syntax.testRawCall"), 2);
	CHECK(runFn<Bool>(L"test.syntax.testCapture"));
	CHECK(runFn<Bool>(L"test.syntax.testRawCapture"));
	CHECK_RUNS(runFn<void>(L"test.syntax.testParams"));
	CHECK(runFn<Bool>(L"test.syntax.testEmpty"));
	CHECK_EQ(runFn<Int>(L"test.syntax.testExpr"), 40);
} END_TEST

using namespace storm::syntax;

static void parse(const wchar *root, const wchar *parse) {
	Package *pkg = gEngine().package(L"test.syntax");
	Parser *p = Parser::create(pkg, root);

	Url *empty = new (p) Url();
	Str *s = new (p) Str(parse);

	p->parse(s, empty);

	if (p->hasError())
		p->throwError();
}

static String parseStr(const wchar *package, const wchar *root, const wchar *parse) {
	Package *pkg = gEngine().package(package);
	Parser *p = Parser::create(pkg, root);

	Url *empty = new (p) Url();
	Str *s = new (p) Str(parse);

	p->parse(s, empty);

	if (p->hasError())
		p->throwError();

	Object *r = p->transform<Object>();
	// PLN(parse << L" => " << r);
	return ::toS(r);
}

static String parseStr(const wchar *root, const wchar *parse) {
	return parseStr(L"test.syntax", root, parse);
}

BEGIN_TEST(ParseOrderTest, BS) {
	CHECK_RUNS(parse(L"Skip", L" a {} b {} c "));

	CHECK_EQ(parseStr(L"Prio", L"a b"), L"ab");
	CHECK_EQ(parseStr(L"Prio", L"var b"), L"b");
	CHECK_EQ(parseStr(L"Prio", L"async b"), L"asyncb");

	CHECK_EQ(parseStr(L"Rec", L"a,b,c"), L"(a)((b)(c))");
	CHECK_EQ(parseStr(L"Rec", L"a.b.c"), L"((a)(b))(c)");
	CHECK_EQ(parseStr(L"Rec", L"a.b.c.d"), L"(((a)(b))(c))(d)");
	CHECK_EQ(parseStr(L"Rec", L"a,b,c,d"), L"(a)((b)((c)(d)))");
	CHECK_EQ(parseStr(L"Rec", L"a.b.c.d.e"), L"((((a)(b))(c))(d))(e)");
	CHECK_EQ(parseStr(L"Rec", L"a,b,c,d,e"), L"(a)((b)((c)((d)(e))))");

	CHECK_EQ(parseStr(L"Rec3", L"a.b.c.d.e.f.g"), L"(((a)(b)(c))(d)(e))(f)(g)");
	CHECK_EQ(parseStr(L"Rec3", L"a,b,c,d,e,f,g"), L"(a)(b)((c)(d)((e)(f)(g)))");

	// Check if ()* is greedy if this fails...
	CHECK_EQ(parseStr(L"Unless", L"a unless a b c"), L"(a)(a(b)(c))");
} END_TEST

// Previous odd crashes in the syntax.
BEGIN_TEST(SyntaxCrashes, BS) {
	CHECK_EQ(::toS(runFn<Name *>(L"test.syntax.complexName")), L"a.b(c, d(e), f)");
} END_TEST
