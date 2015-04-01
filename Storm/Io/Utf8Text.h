#pragma once
#include "Text.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * UTF8 text handling classes.
	 */

	class Utf8Reader : public TextReader {
		STORM_CLASS;
	public:
		// Create the reader.
		STORM_CTOR Utf8Reader(Par<IStream> src);

		// Read a character.
		virtual Nat STORM_FN read();

		// More?
		virtual Bool STORM_FN more();

	private:
		// IStream.
		Auto<IStream> src;

		// Read a character.
		byte readCh();
	};


}
