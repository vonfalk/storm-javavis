#include "stdafx.h"
#include "Compiler/Syntax/Regex.h"

using namespace storm::syntax;

static nat match(const wchar_t *regex, const wchar_t *str, nat start = 0) {
	Engine &e = gEngine();
	Regex r(new (e) Str(regex));
	return r.matchRaw(new (e) Str(str), start);
}

static String tos(const wchar_t *regex) {
	Engine &e = gEngine();
	Regex r(new (e) Str(regex));
	return ::toS(r);
}


BEGIN_TEST(RegexTest, Storm) {
	/**
	 * To string and back.
	 */
	CHECK_EQ(tos(L"[abc]"), L"[a-c]");
	CHECK_EQ(tos(L"[a-c]"), L"[a-c]");
	CHECK_EQ(tos(L"[^abc]"), L"[^a-c]");
	CHECK_EQ(tos(L"[^a-c]"), L"[^a-c]");

	/**
	 * Simple matching.
	 */
	CHECK_EQ(match(L"abc", L"abcd"), 3);
	CHECK_EQ(match(L"abb", L"abcd"), Regex::NO_MATCH);

	/**
	 * Character groups.
	 */
	CHECK_EQ(match(L"[abc]", L"a"), 1);
	CHECK_EQ(match(L"[abc]", L"b"), 1);
	CHECK_EQ(match(L"[abc]", L"c"), 1);
	CHECK_EQ(match(L"[abc]", L"d"), Regex::NO_MATCH);
	CHECK_EQ(match(L"[a-c]", L"a"), 1);
	CHECK_EQ(match(L"[a-c]", L"b"), 1);
	CHECK_EQ(match(L"[a-c]", L"c"), 1);
	CHECK_EQ(match(L"[a-c]", L"d"), Regex::NO_MATCH);
	CHECK_EQ(match(L"[^abc]", L"d"), 1);
	CHECK_EQ(match(L"[^abc]", L"a"), Regex::NO_MATCH);
	CHECK_EQ(match(L"[^abc\\^]", L"^"), Regex::NO_MATCH);

	/**
	 * Escape chars.
	 */
	CHECK_EQ(match(L"a\\.b", L"a.b"), 3);
	CHECK_EQ(match(L"a\\.b", L"abb"), Regex::NO_MATCH);
	CHECK_EQ(match(L"\\[", L"["), 1);
	CHECK_EQ(match(L"[\\^]", L"^"), 1);
	CHECK_EQ(match(L"[\\^]", L"a"), Regex::NO_MATCH);
	CHECK_EQ(match(L"[^\\^]", L"^"), Regex::NO_MATCH);
	CHECK_EQ(match(L"[^\\^]", L"a"), 1);
	CHECK_EQ(match(L"[\\]]", L"]"), 1);

	/**
	 * Check * matching
	 */
	CHECK_EQ(match(L"a*", L"aaaa"), 4);
	CHECK_EQ(match(L"a*c", L"aaac"), 4);
	CHECK_EQ(match(L"a*cd", L"cd"), 2);
	CHECK_EQ(match(L"a*cd", L"acdef"), 3);
	CHECK_EQ(match(L"a*cd", L"aef"), Regex::NO_MATCH);
	CHECK_EQ(match(L"[abc]*d", L"acbd"), 4);
	CHECK_EQ(match(L"[abc]*", L"acbabc"), 6);
	CHECK_EQ(match(L"f[abc]*", L"f"), 1);

	/**
	 * Check ? matching.
	 */
	CHECK_EQ(match(L"a?c", L"c"), 1);
	CHECK_EQ(match(L"a?c", L"ac"), 2);
	CHECK_EQ(match(L"[ab]?c", L"bc"), 2);
	CHECK_EQ(match(L"[ab]?", L"a"), 1);
	CHECK_EQ(match(L"f[ab]?", L"f"), 1);

	/**
	 * . matching
	 */
	CHECK_EQ(match(L"a.c", L"adc"), 3);
	CHECK_EQ(match(L"a.*c", L"afiskd"), Regex::NO_MATCH);
	CHECK_EQ(match(L"a.*c", L"afiskc"), 6);

	/**
	 * + matching.
	 */
	CHECK_EQ(match(L"a+c", L"c"), Regex::NO_MATCH);
	CHECK_EQ(match(L"a+c", L"ac"), 2);
	CHECK_EQ(match(L"a+c", L"aac"), 3);
	CHECK_EQ(match(L"a+", L"aa"), 2);
	CHECK_EQ(match(L"ba+", L"ba"), 2);
	CHECK_EQ(match(L"ba+", L"b"), Regex::NO_MATCH);

	/**
	 * Random tests.
	 */
	CHECK_EQ(match(L"[ab]*bcd..", L"abababcdba"), 10);
	CHECK_EQ(match(L"[ab]*bcd[ab]*", L"abababcdba"), 10);
	CHECK_EQ(match(L" *", L""), 0);

	/**
	 * Check correct indexing when not searching from the start.
	 */
	CHECK_EQ(match(L"abc", L"aabcc", 1), 4);
	CHECK_EQ(match(L"[abc]*", L"dabcd", 1), 4);
	CHECK_EQ(match(L"a", L"zzz", 1), Regex::NO_MATCH);

	/**
	 * Character ranges.
	 */
	CHECK_EQ(match(L"[1-9]*", L"12345"), 5);
	CHECK_EQ(match(L"[1-35-8]*", L"12356"), 5);
	CHECK_EQ(match(L"[1-35-8]*", L"1234"), 3);

	/**
	 * Difference between 0 and NO_MATCH.
	 */
	CHECK_EQ(match(L"a*", L"b"), 0);
	CHECK_EQ(match(L"a+", L"b"), Regex::NO_MATCH);
	CHECK_EQ(match(L"a*", L"zb", 1), 1);
	CHECK_EQ(match(L"a+", L"zb", 1), Regex::NO_MATCH);

	/**
	 * Some real-world examples.
	 */
	CHECK_EQ(match(L"//.*[\n\r]", L"// comment\n"), 11);
	CHECK_EQ(match(L"[ \n\r\t]*//.*[\n\r]", L"   // comment\n"), 14);
	CHECK_EQ(match(L"//.*[\n\r][ \n\r\t]*", L"// comment\n  \n"), 14);
	CHECK_EQ(match(L"[ \n\r\t]*//.*[\n\r][ \n\r\t]*", L"   // comment\n  \n"), 17);
	CHECK_EQ(match(L"[^\\\"\\\\]*", L"Type"), 4);


} END_TEST
