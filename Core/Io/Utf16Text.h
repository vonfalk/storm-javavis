#pragma once
#include "Text.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * UTF16 reader class.
	 */
	class Utf16Reader : public TextReader {
		STORM_CLASS;
	public:
		// Create the reader.
		STORM_CTOR Utf16Reader(IStream *src, Bool byteSwap);

		// Create with a buffer containing the initial few bytes of the stream.
		STORM_CTOR Utf16Reader(IStream *src, Bool byteSwap, Buffer start);

	protected:
		// Read a character.
		virtual Char STORM_FN readChar();

	private:
		// Source stream.
		IStream *src;

		// Buffer.
		Buffer buf;

		// Position in the buffer.
		Nat pos;

		// Swap endianness?
		Bool byteSwap;

		// Read a character.
		nat16 readCh();

		// Read a byte.
		byte readByte();

		// Buffer size.
		enum {
			bufSize = 1024
		};
	};

	/**
	 * UTF16 writer class.
	 */
	class Utf16Writer : public TextWriter {
		STORM_CLASS;
	public:
		// Create the writer.
		STORM_CTOR Utf16Writer(OStream *dest, Bool byteSwap);
		STORM_CTOR Utf16Writer(OStream *dest, TextInfo info, Bool byteSwap);

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

		// Byte-swap output?
		Bool byteSwap;

		// Write bytes to the buffer (automatically flushes it if needed).
		void writeBytes(const byte *data, nat count);

		// Write a wchar to a sequence of bytes in the correct byte order.
		void write(byte *to, wchar ch);

		// Initialize.
		void init();

		// Buffer size.
		enum {
			bufSize = 1024
		};
	};

}
