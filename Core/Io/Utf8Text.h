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
	};

}
