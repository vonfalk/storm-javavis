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
		// Create the reader.
		STORM_CTOR Utf16Reader(Par<IStream> src, Bool bigEndian);

		// Read a character.
		virtual Nat STORM_FN readPoint();

	private:
		// IStream.
		Auto<IStream> src;

		// Big endian?
		Bool bigEndian;

		// Read a character.
		nat16 readCh();
	};

}
