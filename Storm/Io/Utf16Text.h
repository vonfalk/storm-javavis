#pragma once
#include "Text.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * UTF16 text handling classes.
	 */

	class Utf16Reader : public TextReader {
		STORM_CLASS;
	public:
		// Create the reader. 'rev' is if we need to reverse the byte order.
		// TODO: Indicate little or big endian!
		STORM_CTOR Utf16Reader(Par<IStream> src, Bool rev);

		// Read a character.
		virtual Nat STORM_FN read();

		// More?
		virtual Bool STORM_FN more();

	private:
		// IStream.
		Auto<IStream> src;

		// Reverse endianness.
		bool rev;

		// Read a character.
		nat16 readCh();
	};

}
