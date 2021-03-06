#pragma once

#include "TextFile.h"

class TextWriter {
public:
	virtual ~TextWriter();

	virtual textfile::Format format() = 0;

	virtual void put(wchar_t ch) = 0;
	virtual void put(const String &str);

	static TextWriter *create(Stream *stream, bool owner, textfile::Format fmt);
protected:
	TextWriter(Stream *stream, bool owner);

	Stream *stream;
	bool owner;
};

namespace textfile {
	class Utf8Writer : public TextWriter {
	public:
		Utf8Writer(Stream *to, bool owner, bool bom);

		virtual textfile::Format format() { return bom ? textfile::utf8 : textfile::utf8noBom; };
		virtual void put(wchar_t ch);
		using TextWriter::put;
	private:
		bool bom;
		wchar_t largeCp;

		static inline bool isSurrogate(wchar_t ch) { return (ch & 0xFC00) == 0xD800; };
		static int decodeUtf16(wchar_t high, wchar_t low);
		void encodeUtf8(int utf32);
	};

	class Utf16Writer : public TextWriter {
	public:
		Utf16Writer(Stream *to, bool owner, bool reverseEndian = false);

		virtual textfile::Format format() { return (reverseEndian ? textfile::utf16rev : textfile::utf16); };

		virtual void put(wchar_t ch);
		using TextWriter::put;
	private:
		bool reverseEndian;

		void putUtf16(wchar ch);
	};
}

