#include "stdafx.h"
#include "Test/Test.h"
#include "Storm/Io/Text.h"
#include "Storm/Io/MemStream.h"

#include <locale>

// Some data with various sizes of UTF code points used for tests.
// Contains a, 0xD6 (O with dots), 0xD36 (phi), 0x3042 (hiragana A), 0x79C1 (watashi), 0x1F639 (smiling cat face), e
// This will test various length (fit within one utf8, within one utf16 and not within one utf16).
static nat utf32Data[] = { 'a', 0xD6, 0x0D36, 0x3042, 0x79C1, 0x1F639, 'e', 0 };
static nat16 utf16Data[] = { 0xFEFF, 'a', 0xD6, 0x0D36, 0x3042, 0x79C1, 0xD83D, 0xDE39, 'e' };
static nat16 revUtf16Data[] = { 0xFFFE, 'a' << 8, 0xD600, 0x360D, 0x4230, 0xC179, 0x3DD8, 0x39DE, 'e' << 8 };
static byte utf8Data[] = {
	0xEF, 0xBB, 0xBF, // BOM
	'a',
	0xC3, 0x96, // 0xD6
	0xE0, 0xB4, 0xB6, // 0x0D36
	0xE3, 0x81, 0x82, // 0x3042
	0xE7, 0xA7, 0x81, // 0x79C1
	0xF0, 0x9F, 0x98, 0xB9, // 0x1F639
	'e'
};
static byte plainUtf8Data[] = {
	'a',
	0xC3, 0x96, // 0xD6
	0xE0, 0xB4, 0xB6, // 0x0D36
	0xE3, 0x81, 0x82, // 0x3042
	0xE7, 0xA7, 0x81, // 0x79C1
	0xF0, 0x9F, 0x98, 0xB9, // 0x1F639
	'e'
};
static wchar strData[] = L"a\u00D6\u0D36\u3042\u79C1\U0001F639e";

// Compare utf32 from a stream.
bool streamEq(Par<TextReader> from, nat *v) {
	for (nat i = 0; v[i]; i++) {
		Nat r = from->read();
		if (r != v[i]) {
			PLN("Got " << toHex(r) << L", but expected " << toHex(v[i]) << L" at " << i);
			return false;
		}
	}

	return from->read() == 0;
}

Auto<TextReader> read(const Buffer &b) {
	Auto<storm::IStream> s = CREATE(IMemStream, *gEngine, b);
	return readText(s);
}

#define READ(array) read(Buffer(array, sizeof(array)))

BEGIN_TEST_(TextTest) {

	CHECK(streamEq(READ(utf16Data), utf32Data));
	CHECK(streamEq(READ(revUtf16Data), utf32Data));
	CHECK(streamEq(READ(utf8Data), utf32Data));
	CHECK(streamEq(READ(plainUtf8Data), utf32Data));

} END_TEST
