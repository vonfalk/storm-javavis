#include "stdafx.h"

#include "TextWriter.h"

//////////////////////////////////////////////////////////////////////////
// TextWriter
//////////////////////////////////////////////////////////////////////////

TextWriter::TextWriter(Stream *stream, bool owner) : stream(stream), owner(owner) {}

TextWriter::~TextWriter() {
	if (stream && owner)
		delete stream;
}

void TextWriter::put(const String &str) {
	for (nat i = 0; i < str.size(); i++) {
		put(str[i]);
	}
}

TextWriter *TextWriter::create(Stream *stream, bool owner, textfile::Format fmt) {
	switch (fmt) {
	case textfile::utf8:
		return new textfile::Utf8Writer(stream, owner, true);
	case textfile::utf8noBom:
		return new textfile::Utf8Writer(stream, owner, false);
	case textfile::utf16:
		return new textfile::Utf16Writer(stream, owner, false);
	case textfile::utf16rev:
		return new textfile::Utf16Writer(stream, owner, true);
	default:
		assert(false);
		return null;
	}
}

namespace textfile {
	//////////////////////////////////////////////////////////////////////////
	// Utf8Writer
	//////////////////////////////////////////////////////////////////////////

	Utf8Writer::Utf8Writer(Stream *to, bool owner, bool bom) : TextWriter(to, owner), largeCp(0), bom(bom) {
		if (bom)
			put(0xFEFF);
	}

	void Utf8Writer::put(wchar_t ch) {
#ifdef WINDOWS
		if (isSurrogate(largeCp)) {
			int utf32 = decodeUtf16(largeCp, ch);
			encodeUtf8(utf32);
			largeCp = 0;
		} else if (isSurrogate(ch)) {
			largeCp = 0;
		} else {
			encodeUtf8(ch);
		}
#else
		// Assume wchar_t == utf32
		encodeUtf8(ch);
#endif
	}

	int Utf8Writer::decodeUtf16(wchar_t high, wchar_t low) {
		int result = (high & 0x3FF) << 10;
		result |= low & 0x3FF;
		return result + 0x010000;
	}

	void Utf8Writer::encodeUtf8(int utf32) {
		char data[6];
		nat size = 0;
		if ((0x7F & utf32) == utf32) {
			data[0] = (char)utf32;
			size = 1;
		} else if ((0x7FF & utf32) == utf32) {
			data[0] = char(0xC0 | ((utf32 & 0x7C0) >> 6));
			data[1] = char(0x80 | (utf32 & 0x3F));
			size = 2;
		} else if ((0xFFFF & utf32) == utf32) {
			data[0] = char(0xE0 | ((utf32 & 0xF000) >> 12));
			data[1] = char(0x80 | ((utf32 & 0x0FC0) >> 6));
			data[2] = char(0x80 | (utf32 & 0x3F));
			size = 3;
		} else if ((0x1FFFFF & utf32) == utf32) {
			data[0] = char(0xF0 | ((utf32 & 0x1C0000) >> 18));
			data[1] = char(0x80 | ((utf32 & 0x03F000) >> 12));
			data[2] = char(0x80 | ((utf32 & 0x000FC0) >> 6));
			data[3] = char(0x80 | (utf32 & 0x00003F));
			size = 4;
		} else if ((0x3FFFFFF & utf32) == utf32) {
			data[0] = char(0xF7 | ((utf32 & 0x3000000) >> 24));
			data[1] = char(0x80 | ((utf32 & 0x0FC0000) >> 18));
			data[2] = char(0x80 | ((utf32 & 0x003F000) >> 12));
			data[3] = char(0x80 | ((utf32 & 0x0000FC0) >> 6));
			data[4] = char(0x80 | (utf32 & 0x000003F));
			size = 5;
		} else {
			data[0] = char(0xFC | ((utf32 & 0x40000000) >> 30));
			data[1] = char(0xF7 | ((utf32 & 0x3F000000) >> 24));
			data[2] = char(0x80 | ((utf32 & 0x00FC0000) >> 18));
			data[3] = char(0x80 | ((utf32 & 0x0003F000) >> 12));
			data[4] = char(0x80 | ((utf32 & 0x00000FC0) >> 6));
			data[5] = char(0x80 | (utf32 & 0x0000003F));
			size = 6;
		}

		stream->write(size, data);
	}

	//////////////////////////////////////////////////////////////////////////
	// Utf16Writer
	//////////////////////////////////////////////////////////////////////////

	Utf16Writer::Utf16Writer(Stream *to, bool owner, bool reverseEndian) :
		TextWriter(to, owner), reverseEndian(reverseEndian) {

		put(0xFEFF);
	}

	void Utf16Writer::put(wchar_t ch) {
#ifdef WINDOWS
		putUtf16(ch);
#else
		if (ch > 0xFFFF) {
			// Encode a surrogate pair.
			ch -= 0x010000;
			wchar high = 0xD800 | wchar((ch >> 10) & 0x3FF);
			wchar low = 0xDC00 | wchar(ch & 0x3FF);
			putUtf16(high);
			putUtf16(low);
		} else {
			putUtf16(wchar(ch));
		}
#endif
	}

	void Utf16Writer::putUtf16(wchar ch) {
		if (reverseEndian) {
			ch = ((ch & 0xFF00) >> 8) | ((ch & 0x00FF) << 8);
		}
		stream->write(2, &ch);
	}
}

