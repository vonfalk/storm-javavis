#include "stdafx.h"
#include "Test/Test.h"

#include "Storm/Regex.h"

using namespace storm;

BEGIN_TEST(RegexTest) {

	/**
	 * Simple matching
	 */
	CHECK_EQ(matchRegex(L"abc", L"abcd"), 3);
	CHECK_EQ(matchRegex(L"abb", L"abcd"), NO_MATCH);

	/**
	 * Character groups.
	 */
	CHECK_EQ(matchRegex(L"[abc]", L"a"), 1);
	CHECK_EQ(matchRegex(L"[abc]", L"b"), 1);
	CHECK_EQ(matchRegex(L"[abc]", L"c"), 1);
	CHECK_EQ(matchRegex(L"[abc]", L"d"), NO_MATCH);
	CHECK_EQ(matchRegex(L"[^abc]", L"d"), 1);
	CHECK_EQ(matchRegex(L"[^abc]", L"a"), NO_MATCH);
	CHECK_EQ(matchRegex(L"[^abc\\^]", L"^"), NO_MATCH);

	/**
	 * Escape chars.
	 */
	CHECK_EQ(matchRegex(L"a\\.b", L"a.b"), 3);
	CHECK_EQ(matchRegex(L"\\[", L"["), 1);
	CHECK_EQ(matchRegex(L"[\\^]", L"^"), 1);
	CHECK_EQ(matchRegex(L"[\\^]", L"a"), NO_MATCH);
	CHECK_EQ(matchRegex(L"[^\\^]", L"^"), NO_MATCH);
	CHECK_EQ(matchRegex(L"[^\\^]", L"a"), 1);
	CHECK_EQ(matchRegex(L"[\\]]", L"]"), 1);

	/**
	 * Check * matching
	 */
	CHECK_EQ(matchRegex(L"a*c", L"aaac"), 4);
	CHECK_EQ(matchRegex(L"a*cd", L"cd"), 2);
	CHECK_EQ(matchRegex(L"a*cd", L"acdef"), 3);
	CHECK_EQ(matchRegex(L"a*cd", L"aef"), NO_MATCH);
	CHECK_EQ(matchRegex(L"[abc]*d", L"acbd"), 4);
	CHECK_EQ(matchRegex(L"[abc]*", L"acbabc"), 6);
	CHECK_EQ(matchRegex(L"f[abc]*", L"f"), 1);

	/**
	 * Check ? matching.
	 */
	CHECK_EQ(matchRegex(L"a?c", L"c"), 1);
	CHECK_EQ(matchRegex(L"a?c", L"ac"), 2);
	CHECK_EQ(matchRegex(L"[ab]?c", L"bc"), 2);
	CHECK_EQ(matchRegex(L"[ab]?", L"a"), 1);
	CHECK_EQ(matchRegex(L"f[ab]?", L"f"), 1);

	/**
	 * . matching
	 */
	CHECK_EQ(matchRegex(L"a.c", L"adc"), 3);
	CHECK_EQ(matchRegex(L"a.*c", L"afiskd"), NO_MATCH);
	CHECK_EQ(matchRegex(L"a.*c", L"afiskc"), 6);

	/**
	 * + matching.
	 */
	CHECK_EQ(matchRegex(L"a+c", L"c"), NO_MATCH);
	CHECK_EQ(matchRegex(L"a+c", L"ac"), 2);
	CHECK_EQ(matchRegex(L"a+c", L"aac"), 3);
	CHECK_EQ(matchRegex(L"a+", L"aa"), 2);
	CHECK_EQ(matchRegex(L"ba+", L"ba"), 2);
	CHECK_EQ(matchRegex(L"ba+", L"b"), NO_MATCH);

	/**
	 * Random tests.
	 */
	CHECK_EQ(matchRegex(L"[ab]*bcd..", L"abababcdba"), 10);
	CHECK_EQ(matchRegex(L"[ab]*bcd[ab]*", L"abababcdba"), 10);

	/**
	 * Check correct indexing when not searching from the start.
	 */
	CHECK_EQ(matchRegex(L"abc", L"aabcc", 1), 4);
	CHECK_EQ(matchRegex(L"[abc]*", L"dabcd", 1), 4);
	CHECK_EQ(matchRegex(L"a", L"zzz", 1), NO_MATCH);

	/**
	 * Character ranges.
	 */
	CHECK_EQ(matchRegex(L"[1-9]*", L"12345"), 5);
	CHECK_EQ(matchRegex(L"[1-35-8]*", L"12356"), 5);
	CHECK_EQ(matchRegex(L"[1-35-8]*", L"1234"), 3);

	/**
	 * Difference between 0 and NO_MATCH.
	 */
	CHECK_EQ(matchRegex(L"a*", L"b"), 0);
	CHECK_EQ(matchRegex(L"a+", L"b"), NO_MATCH);
	CHECK_EQ(matchRegex(L"a*", L"zb", 1), 1);
	CHECK_EQ(matchRegex(L"a+", L"zb", 1), NO_MATCH);

} END_TEST
