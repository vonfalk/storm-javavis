#pragma once
#include "Text.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * Reading and decoding of UTF8.
	 */
	class Utf8Reader : public TextReader {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR Utf8Reader(IStream *src);

		// Create with initial buffer contents.
		STORM_CTOR Utf8Reader(IStream *src, Buffer start);

	protected:
		virtual Char STORM_FN readChar();

	private:
		// Source stream.
		IStream *src;

		// Input buffer.
		Buffer buf;

		// Position in the buffer.
		Nat pos;

		// Read a character.
		Byte readByte();

		// Reset one byte.
		void ungetByte();

		// Buffer size.
		enum {
			bufSize = 1024
		};
	};


	/**
	 * Writing and encoding of UTF8.
	 */
	class Utf8Writer : public TextWriter {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR Utf8Writer(OStream *to);
		STORM_CTOR Utf8Writer(OStream *to, TextInfo info);

		// Flush output.
		virtual void STORM_FN flush();

	protected:
		// Write character.
		virtual void STORM_FN writeChar(Char ch);

	private:
		// Target stream.
		OStream *dest;

		// Output buffer.
		Buffer buf;

		// Position in the buffer.
		Nat pos;

		// Write bytes to the buffer (automatically flushes it if needed).
		void writeBytes(const byte *data, nat count);

		// Initialize.
		void init();

		// Buffer size.
		enum {
			bufSize = 1024
		};
	};

}
