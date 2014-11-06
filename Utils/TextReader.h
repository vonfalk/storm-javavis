#pragma once

#include "Stream.h"
#include "TextFile.h"

//////////////////////////////////////////////////////////////////////////
// Supports Utf8 and Utf16, little and big endian.
//////////////////////////////////////////////////////////////////////////

class TextReader {
public:
	virtual ~TextReader();

	virtual textfile::Format format() = 0;

	virtual wchar_t get() = 0;
	virtual void seek(nat position) { stream->seek(position); };
	inline nat position() const { return nat(stream->pos()); };
	inline bool atEnd() const { return !stream->more(); };
	inline bool more() const { return stream->more(); };

	String getLine();
	String getAll();

	inline Stream *getStream() const { return stream; };

	// Takes ownership of 'stream'.
	static TextReader *create(Stream *stream);
protected:
	TextReader(Stream *stream);

	Stream *stream;
};


namespace textfile {
	class Utf8Reader : public TextReader {
	public:
		Utf8Reader(Stream *stream, bool bom);
		virtual ~Utf8Reader();

		virtual textfile::Format format() { return bom ? textfile::utf8 : textfile::utf8noBom; };

		virtual void seek(nat position);

		virtual wchar_t get();
	private:
		bool bom;
		bool nextValid;
		wchar_t nextCh;
		byte read();
		byte peek();

		wchar_t readChar();
		wchar_t encodeUtf16(nat utf32);
		nat readChar(byte first, nat numToRead);
		nat decodeUtf8();
	};

	class Utf16Reader : public TextReader {
	public:
		Utf16Reader(Stream *stream, bool reverseEndian);
		virtual ~Utf16Reader();

		virtual textfile::Format format() { return (reverseEndian ? textfile::utf16rev : textfile::utf16); };

		virtual wchar_t get();
	private:
		bool reverseEndian;
	};
}

