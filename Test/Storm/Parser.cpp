#include "stdafx.h"
#include "Compiler/Package.h"
#include "Compiler/Reader.h"
#include "Compiler/FileReader.h"
#include "Compiler/Syntax/Parser.h"
#include "Compiler/Syntax/Earley/Parser.h"
#include "Compiler/Syntax/GLR/Parser.h"
#include "Core/Timing.h"
#include "Core/Convert.h"
#include "Core/Io/Text.h"
#include "Fn.h"

using syntax::Parser;
using namespace storm::syntax;

static void parse(const wchar_t *root, const wchar_t *parse, Nat backend) {
	Package *pkg = gEngine().package(S("test.syntax"));
#ifdef WINDOWS
	Parser *p = Parser::create(pkg, root, createBackend(gEngine(), backend));
#else
	const wchar *r = toWChar(gEngine(), root)->v;
	Parser *p = Parser::create(pkg, r, createBackend(gEngine(), backend));
#endif

	// PVAR(parse);
	Url *empty = new (p) Url();
	Str *s = new (p) Str(parse);

	p->parse(s, empty);
	if (p->hasError())
		p->throwError();
}

static bool parseC(const wchar_t *root, const wchar_t *parse, Nat backend) {
	Package *pkg = gEngine().package(S("test.syntax.context"));
#ifdef WINDOWS
	Parser *p = Parser::create(pkg, root, createBackend(gEngine(), backend));
#else
	const wchar *r = toWChar(gEngine(), root)->v;
	Parser *p = Parser::create(pkg, r, createBackend(gEngine(), backend));
#endif

	// PVAR(parse);
	Url *empty = new (p) Url();
	Str *s = new (p) Str(parse);

	p->parse(s, empty);
	return !p->hasError();
}

static String parseStr(const wchar_t *package, const wchar_t *root, const wchar_t *parse, Nat backend) {
#ifdef WINDOWS
	Package *pkg = gEngine().package(package);
	Parser *p = Parser::create(pkg, root, createBackend(gEngine(), backend));
#else
	Package *pkg = gEngine().package(toWChar(gEngine(), package)->v);
	Parser *p = Parser::create(pkg, toWChar(gEngine(), root)->v, createBackend(gEngine(), backend));
#endif

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

static String parseStr(const wchar_t *root, const wchar_t *parse, Nat backend) {
	return parseStr(L"test.syntax", root, parse, backend);
}

BEGIN_TEST(ParserTest, Storm) {
	Engine &e = gEngine();

	Package *pkg = as<Package>(e.scope().find(parseSimpleName(e, S("test.grammar"))));
	VERIFY(pkg);

	for (Nat id = 0; id < backendCount(); id++) {
		{
			// Plain sentences.
			Parser *p = Parser::create(pkg, S("Sentence"), createBackend(e, id));
			Str *s = new (e) Str(S("the cat runs"));
			CHECK(p->parse(s, new (e) Url()));
			CHECK(!p->hasError());
			CHECK(p->hasTree());
			CHECK(p->matchEnd() == s->end());

			syntax::Node *tree = p->tree();
			Str *r = syntax::transformNode<Str>(tree);
			CHECK_EQ(::toS(r), L"cat");
			CHECK_RUNS(p->infoTree());

			CHECK(p->parse(new (e) Str(S("the cat runs!")), new (e) Url()));
			CHECK(p->hasError());
			CHECK(p->hasTree());
			CHECK_EQ(p->matchEnd().v(), Char('!'));
			CHECK_RUNS(p->infoTree());
		}

		{
			// Repetitions.
			Parser *p = Parser::create(pkg, S("Sentences"), createBackend(e, id));
			Str *s = new (e) Str(S("the cat runs. the bird sleeps. the dog swims."));
			CHECK(p->parse(s, new (e) Url()));
			CHECK(!p->hasError());
			CHECK(p->hasTree());
			CHECK(p->matchEnd() == s->end());

			syntax::Node *tree = p->tree();
			Array<Str *> *r = syntax::transformNode<Array<Str *>>(tree);
			CHECK_EQ(::toS(r), L"[cat, bird, dog]");
			CHECK_RUNS(p->infoTree());
		}

		{
			// Captures.
			Parser *p = Parser::create(pkg, S("WholeSentence"), createBackend(e, id));
			Str *s = new (e) Str(S("the cat runs."));
			CHECK(p->parse(s, new (e) Url()));
			CHECK(!p->hasError());
			CHECK(p->hasTree());
			CHECK(p->matchEnd() == s->end());

			syntax::Node *tree = p->tree();
			Str *r = syntax::transformNode<Str>(tree);
			CHECK_EQ(::toS(r), L"the cat runs");
			CHECK_RUNS(p->infoTree());

			CHECK(p->parse(new (e) Str(S(".the cat runs.")), new(e) Url()));
			CHECK(!p->hasError());
			CHECK(p->hasTree());
			tree = p->tree();
			r = syntax::transformNode<Str>(tree);
			CHECK_EQ(::toS(r), L"the cat runs");
			CHECK_RUNS(p->infoTree());
		}
	}

} END_TEST

BEGIN_TEST(ParserExt, BS) {
	// Extensible syntax in the parser.
	Engine &e = gEngine();

	Package *pkg = as<Package>(e.scope().find(parseSimpleName(e, S("test.syntax"))));
	VERIFY(pkg);

	Parser *p = Parser::create(pkg, S("SCommaList"));
	Str *s = new (e) Str(S("(a, b)"));
	CHECK(p->parse(s, new (e) Url()));
	CHECK(!p->hasError());
	CHECK(p->hasTree());

	syntax::Node *tree = p->tree();
	Array<Str *> *r = syntax::transformNode<Array<Str *>>(tree);
	CHECK_EQ(::toS(r), L"[a, b]");

	s = new (e) Str(S("()"));
	CHECK(p->parse(s, new (e) Url()));
	CHECK(!p->hasError());
	CHECK(p->hasTree());

	tree = p->tree();
	r = syntax::transformNode<Array<Str *>>(tree);
	CHECK_EQ(::toS(r), L"[]");
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
	Engine &e = gEngine();

	CHECK_RUNS(runFn<void>(S("test.syntax.testSimple")));
	CHECK(runFn<Bool>(S("test.syntax.testSentence")));
	CHECK_EQ(toS(runFn<Str *>(S("test.syntax.testManualTfm"), new (e) Str(S("dogs sleep")))), L"Plural dog");
	CHECK_EQ(toS(runFn<Str *>(S("test.syntax.testManualTfm"), new (e) Str(S("the dog sleeps")))), L"Singular dog");
	CHECK_EQ(toS(runFn<Str *>(S("test.syntax.testManualTfm"), new (e) Str(S("do dogs sleep?")))), L"Question dog");
	CHECK(runFn<Bool>(S("test.syntax.testMaybe")));
	CHECK(runFn<Bool>(S("test.syntax.testArray")));
	CHECK(runFn<Bool>(S("test.syntax.testCall")));
	CHECK(runFn<Bool>(S("test.syntax.testCallMaybe")));
	CHECK_EQ(runFn<Nat>(S("test.syntax.testRaw")), 2);
	CHECK_EQ(runFn<Nat>(S("test.syntax.testRawCall")), 2);
	CHECK(runFn<Bool>(S("test.syntax.testCapture")));
	CHECK(runFn<Bool>(S("test.syntax.testRawCapture")));
	CHECK_RUNS(runFn<void>(S("test.syntax.testParams")));
	CHECK(runFn<Bool>(S("test.syntax.testEmpty")));
	CHECK_EQ(runFn<Int>(S("test.syntax.testExpr")), 40);
} END_TEST

// Previous odd crashes in the syntax.
BEGIN_TEST(SyntaxCrashes, BS) {
	CHECK_EQ(::toS(runFn<Name *>(S("test.syntax.complexName"))), L"a.b(c, d(e), f)");
} END_TEST

/**
 * Test behavior regarding non-context-free grammar.
 */
BEGIN_TEST(SyntaxContext, BS) {
	Nat parser = 0; // GLR

	// Should be accepted.
	CHECK(parseC(L"SBlock", L"{ a b }", parser));

	// We're using 'c' inside a regular block. Should fail!
	CHECK(!parseC(L"SBlock", L"{ a c }", parser));

	// So should using 'd'.
	CHECK(!parseC(L"SBlock", L"{ a d }", parser));

	// But using them inside their respective blocks should be fine!
	CHECK(parseC(L"SBlock", L"{ a extra c { a c } b }", parser));
	CHECK(parseC(L"SBlock", L"{ a extra d { a d } b }", parser));

	// They should not overlap...
	CHECK(!parseC(L"SBlock", L"{ extra c { d } }", parser));
	CHECK(!parseC(L"SBlock", L"{ extra d { c } }", parser));

	// But this should be allowed...
	CHECK(parseC(L"SBlock", L"{ extra c { extra d { c d } } }", parser));
	CHECK(parseC(L"SBlock", L"{ extra d { extra c { c d } } }", parser));

	// Check so that the position of the error is proper as well. Should return the first error.
	{
		Package *pkg = gEngine().package(S("test.syntax.context"));
		Parser *p = Parser::create(pkg, S("SBlock"), createBackend(gEngine(), parser));
		Url *empty = new (p) Url();
		Str *s = new (p) Str(S("{ extra c { c d d } }"));
		p->parse(s, empty);

		CHECK(p->hasError());
		CHECK_EQ(p->error().where.pos, 14);
	}

	// Make sure the context parameter to 'parseApprox' works properly.
	{
		Package *pkg = gEngine().package(S("test.syntax.context"));
		InfoParser *p = InfoParser::create(pkg, S("SBlock"), createBackend(gEngine(), parser));
		Rule *dep = as<Rule>(gEngine().scope().find(parseSimpleName(gEngine(), S("test.syntax.context.SExtraC"))));
		VERIFY(dep);

		Url *empty = new (p) Url();
		Str *s = new (p) Str(S("{ c a }"));

		// Should be considered erroneous. We don't have any context yet!
		CHECK(p->parseApprox(s, empty).any());

		// But, if we add the context, it should be fine!
		Set<Rule *> *ctx = new (p) Set<Rule *>();
		ctx->put(dep);
		CHECK(!p->parseApprox(s, empty, ctx).any());
	}
} END_TEST


/**
 * Test performance of the parsers by parsing a large bs-file.
 */
BEGIN_TESTX(ParserPerformance, BS) {
	Engine &e = gEngine();

	Package *root = e.package();
	Package *pkg = e.package(S("test.large"));
	VERIFY(pkg);

	Url *file = pkg->url()->push(new (e) Str(S("eval.bs")));
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
