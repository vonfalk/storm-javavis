#include "stdafx.h"
#include "Test/Test.h"

#include "Storm/Regex.h"

using namespace storm;

BEGIN_TEST(RegexTest) {

	/**
	 * Simple matching
	 */
	CHECK_EQ(matchRegex(L"abc", L"abcd"), 3);
	CHECK_EQ(matchRegex(L"abb", L"abcd"), 0);

	/**
	 * Character groups.
	 */
	CHECK_EQ(matchRegex(L"[abc]", L"a"), 1);
	CHECK_EQ(matchRegex(L"[abc]", L"b"), 1);
	CHECK_EQ(matchRegex(L"[abc]", L"c"), 1);
	CHECK_EQ(matchRegex(L"[abc]", L"d"), 0);
	CHECK_EQ(matchRegex(L"[^abc]", L"d"), 1);
	CHECK_EQ(matchRegex(L"[^abc]", L"a"), 0);
	CHECK_EQ(matchRegex(L"[^abc\\^]", L"^"), 0);

	/**
	 * Escape chars.
	 */
	CHECK_EQ(matchRegex(L"\\[", L"["), 1);
	CHECK_EQ(matchRegex(L"[\\^]", L"^"), 1);
	CHECK_EQ(matchRegex(L"[\\^]", L"a"), 0);
	CHECK_EQ(matchRegex(L"[^\\^]", L"^"), 0);
	CHECK_EQ(matchRegex(L"[^\\^]", L"a"), 1);
	CHECK_EQ(matchRegex(L"[\\]]", L"]"), 1);

	/**
	 * Check * matching
	 */
	CHECK_EQ(matchRegex(L"a*c", L"aaac"), 4);
	CHECK_EQ(matchRegex(L"a*cd", L"cd"), 2);
	CHECK_EQ(matchRegex(L"[abc]*d", L"acbd"), 4);

	/**
	 * Check ? matching.
	 */
	CHECK_EQ(matchRegex(L"a?c", L"c"), 1);
	CHECK_EQ(matchRegex(L"a?c", L"ac"), 2);
	CHECK_EQ(matchRegex(L"[ab]?c", L"bc"), 2);

	/**
	 * . matching
	 */
	CHECK_EQ(matchRegex(L"a.c", L"adc"), 3);
	CHECK_EQ(matchRegex(L"a.*c", L"afiskd"), 0);
	CHECK_EQ(matchRegex(L"a.*c", L"afiskc"), 6);

	/**
	 * + matching.
	 */
	CHECK_EQ(matchRegex(L"a+c", L"c"), 0);
	CHECK_EQ(matchRegex(L"a+c", L"ac"), 2);
	CHECK_EQ(matchRegex(L"a+c", L"aac"), 3);

} END_TEST
