#include "stdafx.h"
#include "Test/Test.h"
#include "Storm/Io/Text.h"
#include "Storm/Io/MemStream.h"
#include "Storm/Io/Utf8Text.h"
#include "Storm/Io/Utf16Text.h"

#include <locale>

// Some data with various sizes of UTF code points used for tests.
// Contains a, 0xD6 (O with dots), 0xD36 (phi), 0x3042 (hiragana A), 0x79C1 (watashi), 0x1F639 (smiling cat face), e
// This will test various length (fit within one utf8, within one utf16 and not within one utf16).
static nat utf32Data[] = { 0x61, 0xD6, 0x0D36, 0x3042, 0x79C1, 0x1F639, 0x65, 0 };
static nat16 utf16Data[] = { 0xFEFF, 0x61, 0xD6, 0x0D36, 0x3042, 0x79C1, 0xD83D, 0xDE39, 0x65 };
static nat16 revUtf16Data[] = { 0xFFFE, 0x6100, 0xD600, 0x360D, 0x4230, 0xC179, 0x3DD8, 0x39DE, 0x6500 };
static byte utf8Data[] = {
	0xEF, 0xBB, 0xBF, // BOM
	0x61, // a
	0xC3, 0x96, // 0xD6
	0xE0, 0xB4, 0xB6, // 0x0D36
	0xE3, 0x81, 0x82, // 0x3042
	0xE7, 0xA7, 0x81, // 0x79C1
	0xF0, 0x9F, 0x98, 0xB9, // 0x1F639
	0x65, // e
};
static byte plainUtf8Data[] = {
	0x61, // a
	0xC3, 0x96, // 0xD6
	0xE0, 0xB4, 0xB6, // 0x0D36
	0xE3, 0x81, 0x82, // 0x3042
	0xE7, 0xA7, 0x81, // 0x79C1
	0xF0, 0x9F, 0x98, 0xB9, // 0x1F639
	0x65, // e
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

void copyText(Par<TextReader> src, Par<TextWriter> to, bool addBom) {
	if (addBom)
		to->write(0xFEFF);

	while (Nat p = src->read()) {
		to->write(p);
	}
}

template <class T>
bool checkBuffer(const Buffer &buf, T *ref, nat size) {
	if (size != buf.count()) {
		PLN("Size mismatch: " << buf.count() << " vs " << size);
		PVAR(buf);
		return false;
	}

	for (nat i = 0; i < size; i += sizeof(T)) {
		T tmp;
		memcpy(&tmp, &buf[i], sizeof(T));

		if (tmp != ref[i / sizeof(T)]) {
			PLN("Got " << toHex(tmp) << ", but expected " << toHex(ref[i / sizeof(T)]) << " at " << i);
			PVAR(buf);
			return false;
		}
	}

	return true;
}

#define CHECK_BUFFER(buffer, src) CHECK_TITLE(checkBuffer(buffer, src, sizeof(src)), #buffer " == " #src);

BEGIN_TEST_(TextTest) {

	CHECK(streamEq(READ(utf16Data), utf32Data));
	CHECK(streamEq(READ(revUtf16Data), utf32Data));
	CHECK(streamEq(READ(utf8Data), utf32Data));
	CHECK(streamEq(READ(plainUtf8Data), utf32Data));
	CHECK_OBJ_EQ(READ(utf16Data)->readLine(), CREATE(Str, *gEngine, String(strData)));

	{
		Auto<OMemStream> to = CREATE(OMemStream, *gEngine);
		Auto<Utf8Writer> w = CREATE(Utf8Writer, *gEngine, to);
		copyText(READ(utf16Data), w, false);
		CHECK_BUFFER(to->buffer(), plainUtf8Data);
	}

	{
		Auto<OMemStream> to = CREATE(OMemStream, *gEngine);
		Auto<Utf8Writer> w = CREATE(Utf8Writer, *gEngine, to);
		copyText(READ(utf16Data), w, true);
		CHECK_BUFFER(to->buffer(), utf8Data);
	}

	bool littleEndian = false;
#ifdef LITTLE_ENDIAN
	littleEndian = true;
#endif

	{
		Auto<OMemStream> to = CREATE(OMemStream, *gEngine);
		Auto<Utf16Writer> w = CREATE(Utf16Writer, *gEngine, to, littleEndian);
		copyText(READ(utf16Data), w, true);
		CHECK_BUFFER(to->buffer(), utf16Data);
	}

	{
		Auto<OMemStream> to = CREATE(OMemStream, *gEngine);
		Auto<Utf16Writer> w = CREATE(Utf16Writer, *gEngine, to, !littleEndian);
		copyText(READ(utf16Data), w, true);
		CHECK_BUFFER(to->buffer(), revUtf16Data);
	}

} END_TEST
