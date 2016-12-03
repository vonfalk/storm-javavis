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

}
