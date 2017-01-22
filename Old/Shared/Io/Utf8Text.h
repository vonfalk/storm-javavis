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
		virtual Nat STORM_FN readPoint();

	private:
		// IStream.
		Auto<IStream> src;

		// Read a character.
		byte readCh();
	};


	class Utf8Writer : public TextWriter {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR Utf8Writer(Par<OStream> to);

		// Write a character.
		virtual void STORM_FN write(Nat codepoint);

		using TextWriter::write;

	private:
		// OStream.
		Auto<OStream> to;
	};

}
