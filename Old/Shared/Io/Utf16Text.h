#pragma once
#include "Text.h"
#include "Shared/Utf.h"

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


	class Utf16Writer : public TextWriter {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR Utf16Writer(Par<OStream> to, Bool bigEndian);

		// Write a character.
		virtual void STORM_FN write(Nat codepoint);

		using TextWriter::write;

	private:
		// OStream.
		Auto<OStream> to;

		// Big endian?
		Bool bigEndian;
	};

}
