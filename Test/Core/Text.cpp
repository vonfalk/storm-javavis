#include "stdafx.h"
#include "Core/Io/Text.h"
#include "Core/Io/MemStream.h"

// Some data with various sizes of UTF code points used for tests.

// Strings with only ascii characters
static const wchar strData[] = L"new\nline";
static const nat16 utf16Data[] = { 0xFEFF, 0x6E, 0x65, 0x77, 0x0A, 0x6C, 0x69, 0x6E, 0x65 };
static const nat16 revUtf16Data[] = { 0xFFFE, 0x6E00, 0x6500, 0x7700, 0x0A00, 0x6C00, 0x6900, 0x6E00, 0x6500 };
static const byte utf8Data[] = { 0xEF, 0xBB, 0xBF, 0x6E, 0x65, 0x77, 0x0A, 0x6C, 0x69, 0x6E, 0x65 };
static const byte plainUtf8Data[] = { 0x6E, 0x65, 0x77, 0x0A, 0x6C, 0x69, 0x6E, 0x65 };

// Contains a, 0xD6 (O with dots), 0xD36 (phi), 0x3042 (hiragana A), 0x79C1 (watashi), 0x1F639 (smiling cat face), e
// This will test various length (fit within one utf8, within one utf16 and not within one utf16).
static const nat utf32Data2[] = { 0x61, 0xD6, 0x0D36, 0x3042, 0x79C1, 0x1F639, 0x65, 0 };
static const nat16 utf16Data2[] = { 0xFEFF, 0x61, 0xD6, 0x0D36, 0x3042, 0x79C1, 0xD83D, 0xDE39, 0x65 };
static const nat16 revUtf16Data2[] = { 0xFFFE, 0x6100, 0xD600, 0x360D, 0x4230, 0xC179, 0x3DD8, 0x39DE, 0x6500 };
static const byte utf8Data2[] = {
	0xEF, 0xBB, 0xBF, // BOM
	0x61, // a
	0xC3, 0x96, // 0xD6
	0xE0, 0xB4, 0xB6, // 0x0D36
	0xE3, 0x81, 0x82, // 0x3042
	0xE7, 0xA7, 0x81, // 0x79C1
	0xF0, 0x9F, 0x98, 0xB9, // 0x1F639
	0x65, // e
};
static const byte plainUtf8Data2[] = {
	0x61, // a
	0xC3, 0x96, // 0xD6
	0xE0, 0xB4, 0xB6, // 0x0D36
	0xE3, 0x81, 0x82, // 0x3042
	0xE7, 0xA7, 0x81, // 0x79C1
	0xF0, 0x9F, 0x98, 0xB9, // 0x1F639
	0x65, // e
};
static const wchar strData2[] = L"a\u00D6\u0D36\u3042\u79C1\U0001F639e";

TextInput *read(const void *src, nat count) {
	Buffer b = buffer(gEngine(), (const byte *)src, count);
	IMemStream *s = new (gEngine()) IMemStream(b);
	return readText(s);
}

#define READ(array) read(array, sizeof(array))

static String debug(Char ch) {
	return toHex(ch.leading()) + toHex(ch.trailing());
}

static String verify(TextInput *src, const nat *ref) {
	while (true) {
		Char s = src->read();
		if (s == Char(Nat(0))) {
			if (src->more())
				return L"More says there is more data even if 'read' returned 0.";
			break;
		}

		if (*ref == 0)
			return L"Extra characters introduced!";

		if (s != Char(*ref))
			return L"Character mismatch: " + debug(s) + L", expected " + debug(Char(*ref));
		ref++;
	}

	if (*ref != 0)
		return L"Missing characters!";

	return L"";
}

BEGIN_TEST(TestTextInput, Core) {
	Engine &e = gEngine();

	// 'easy' cases:
	CHECK_EQ(::toS(READ(plainUtf8Data)->readAll()), strData);
	CHECK_EQ(::toS(READ(utf8Data)->readAll()), strData);
	CHECK_EQ(::toS(READ(utf16Data)->readAll()), strData);
	CHECK_EQ(::toS(READ(revUtf16Data)->readAll()), strData);

	// Check so we can read lines:
	{
		TextInput *r = READ(utf8Data);
		CHECK_EQ(::toS(r->readLine()), L"new");
		CHECK_EQ(::toS(r->readLine()), L"line");
	}

	// Robust with different line-endings?
	{
		static const char windows[] = "new\r\nline";
		TextInput *r = READ(windows);
		CHECK_EQ(::toS(r->readLine()), L"new");
		CHECK_EQ(::toS(r->readLine()), L"line");

		CHECK_EQ(::toS(READ(windows)->readAll()), L"new\nline");
	}

	{
		static const char mac[] = "new\rline";
		TextInput *r = READ(mac);
		CHECK_EQ(::toS(r->readLine()), L"new");
		CHECK_EQ(::toS(r->readLine()), L"line");

		CHECK_EQ(::toS(READ(mac)->readAll()), L"new\nline");
	}

	// 'hard' cases.
	CHECK_EQ(verify(READ(plainUtf8Data2), utf32Data2), L"");
	CHECK_EQ(verify(READ(utf8Data2), utf32Data2), L"");
	CHECK_EQ(verify(READ(utf16Data2), utf32Data2), L"");
	CHECK_EQ(verify(READ(revUtf16Data2), utf32Data2), L"");

} END_TEST
