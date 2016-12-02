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

	protected:
		// Read a character.
		virtual Char STORM_FN readChar();

	private:
		// Source stream.
		IStream *src;

		// Buffer (only stores 2 bytes).
		Buffer buf;

		// Swap endianness?
		Bool byteSwap;

		// Read a character.
		nat16 readCh();
	};

}
