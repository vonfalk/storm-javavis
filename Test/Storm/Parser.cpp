#include "stdafx.h"
#include "Compiler/Package.h"
#include "Compiler/Reader.h"
#include "Compiler/FileReader.h"
#include "Compiler/Syntax/Parser.h"
#include "Compiler/Syntax/Earley/Parser.h"
#include "Compiler/Syntax/GLR/Parser.h"
#include "Core/Timing.h"
#include "Core/Io/Text.h"
#include "Fn.h"

using syntax::Parser;
using namespace storm::syntax;

static void parse(const wchar *root, const wchar *parse, Nat backend) {
	Package *pkg = gEngine().package(L"test.syntax");
	Parser *p = Parser::create(pkg, root, createBackend(gEngine(), backend));

	// PVAR(parse);
	Url *empty = new (p) Url();
	Str *s = new (p) Str(parse);

	p->parse(s, empty);
	if (p->hasError())
		p->throwError();
}

static String parseStr(const wchar *package, const wchar *root, const wchar *parse, Nat backend) {
	Package *pkg = gEngine().package(package);
	Parser *p = Parser::create(pkg, root, createBackend(gEngine(), backend));

	// PVAR(parse);
	Url *empty = new (p) Url();
	Str *s = new (p) Str(parse);

	p->parse(s, empty);

	if (p->hasError())
		p->throwError();

	Object *r = p->transform<Object>();
	// PLN(parse << L" => " << r);
	return ::toS(r);
}

static String parseStr(const wchar *root, const wchar *parse, Nat backend) {
	return parseStr(L"test.syntax", root, parse, backend);
}

BEGIN_TEST(ParserTest, Storm) {
	Engine &e = gEngine();

	Package *pkg = as<Package>(e.scope().find(parseSimpleName(e, L"test.grammar")));
	VERIFY(pkg);

	for (Nat id = 0; id < backendCount(); id++) {
		{
			// Plain sentences.
			Parser *p = Parser::create(pkg, L"Sentence", createBackend(gEngine(), id));
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

		{
			// Repetitions.
			Parser *p = Parser::create(pkg, L"Sentences", createBackend(gEngine(), id));
			Str *s = new (e) Str(L"the cat runs. the bird sleeps. the dog swims.");
			CHECK(p->parse(s, new (e) Url()));
			CHECK(!p->hasError());
			CHECK(p->hasTree());
			CHECK(p->matchEnd() == s->end());

			syntax::Node *tree = p->tree();
			Array<Str *> *r = syntax::transformNode<Array<Str *>>(tree);
			CHECK_EQ(::toS(r), L"[cat, bird, dog]");
		}

		{
			// Captures.
			Parser *p = Parser::create(pkg, L"WholeSentence", createBackend(gEngine(), id));
			Str *s = new (e) Str(L"the cat runs.");
			CHECK(p->parse(s, new (e) Url()));
			CHECK(!p->hasError());
			CHECK(p->hasTree());
			CHECK(p->matchEnd() == s->end());

			syntax::Node *tree = p->tree();
			Str *r = syntax::transformNode<Str>(tree);
			CHECK_EQ(::toS(r), L"the cat runs");

			CHECK(p->parse(new (e) Str(L".the cat runs."), new(e) Url()));
			CHECK(!p->hasError());
			CHECK(p->hasTree());
			tree = p->tree();
			r = syntax::transformNode<Str>(tree);
			CHECK_EQ(::toS(r), L"the cat runs");
		}
	}

} END_TEST

BEGIN_TEST(ParseTricky, BS) {
	// Tricky cases.
	for (Nat id = 0; id < backendCount(); id++) {
		CHECK_RUNS(parse(L"MultiWS", L"ab", id));
		CHECK_RUNS(parse(L"Skip", L" a {} b {} c ", id));
	}
} END_TEST


BEGIN_TEST(ParseOrderTest, BS) {
	for (Nat i = 0; i < backendCount(); i++) {
		CHECK_EQ(parseStr(L"Prio", L"a b", i), L"ab");
		CHECK_EQ(parseStr(L"Prio", L"var b", i), L"b");
		CHECK_EQ(parseStr(L"Prio", L"async b", i), L"asyncb");

		CHECK_EQ(parseStr(L"SpawnExpr", L"var x = spawn foo", i), L"x(spawnfoo)");
		CHECK_EQ(parseStr(L"SpawnExpr", L"vvv x = spawn foo", i), L"vvv x(spawnfoo)");
		CHECK_EQ(parseStr(L"SpawnExpr", L"var x = foo", i), L"x(foo)");
		CHECK_EQ(parseStr(L"SpawnExpr", L"vvv x = foo", i), L"vvv x(foo)");

		CHECK_EQ(parseStr(L"Rec", L"a.b.c", i), L"((a)(b))(c)");
		CHECK_EQ(parseStr(L"Rec", L"a,b,c", i), L"(a)((b)(c))");
		CHECK_EQ(parseStr(L"Rec", L"a.b.c.d", i), L"(((a)(b))(c))(d)");
		CHECK_EQ(parseStr(L"Rec", L"a,b,c,d", i), L"(a)((b)((c)(d)))");
		CHECK_EQ(parseStr(L"Rec", L"a.b.c.d.e", i), L"((((a)(b))(c))(d))(e)");
		CHECK_EQ(parseStr(L"Rec", L"a,b,c,d,e", i), L"(a)((b)((c)((d)(e))))");
		CHECK_EQ(parseStr(L"Rec3", L"a.b.c.d.e.f.g", i), L"(((a)(b)(c))(d)(e))(f)(g)");
		CHECK_EQ(parseStr(L"Rec3", L"a,b,c,d,e,f,g", i), L"(a)(b)((c)(d)((e)(f)(g)))");

		// Check if ()* is greedy if this fails...
		CHECK_EQ(parseStr(L"Unless", L"a unless a b c", i), L"(a)(a(b)(c))");
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

// Previous odd crashes in the syntax.
BEGIN_TEST(SyntaxCrashes, BS) {
	CHECK_EQ(::toS(runFn<Name *>(L"test.syntax.complexName")), L"a.b(c, d(e), f)");
} END_TEST


/**
 * Test performance of the parsers by parsing a large bs-file.
 */
BEGIN_TEST_(ParserPerformance, BS) {
	Engine &e = gEngine();

	Package *root = e.package();
	Package *pkg = e.package(L"test.large");
	VERIFY(pkg);

	Url *file = pkg->url()->push(new (e) Str(L"eval.bs"));
	Moment start;
	Str *src = readAllText(file);
	Moment end;
	PLN(L"Loaded " << file << L" in " << (end - start));

	PkgReader *reader = createReader(new (e) Array<Url *>(1, file), root);
	VERIFY(reader);

	FileReader *part = reader->readFile(file, src);
	VERIFY(part);
	start = Moment();
	part = part->next(qParser);
	end = Moment();
	VERIFY(part);
	PLN(L"Includes parsed in " << (end - start));

	InfoParser *parser = part->createParser();
	start = Moment();
	CHECK(parser->parse(src, file));
	end = Moment();

	PLN(L"Parsed " << file << L" in " << (end - start));
} END_TEST
