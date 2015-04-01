#pragma once
#include "Stream.h"
#include "Storm/Lib/Str.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * Text IO. Supports UTF8 and UTF16, little and big endian. The package includes
	 * a method of detecting which encoding is used. TODO: this should also be extensible.
	 *
	 * TODO: Better handling of line endings!
	 * TODO: Find a good way of duplicating an input format to an output format.
	 */

	class TextReader : public Object {
		STORM_CLASS;
	public:
		// Ctor.
		STORM_CTOR TextReader();

		// Read a single code point from the stream, encoded in UTF32. Returns the 0 character on failure.
		virtual Nat STORM_FN read();

		// Read an entire line from the file (does not care about line endings).
		virtual Str *STORM_FN readLine();

		// Read the entire file.
		virtual Str *STORM_FN readAll();

		// More data?
		virtual Bool STORM_FN more();
	};


	class TextWriter : public Object {
		STORM_CLASS;
	public:
		// Ctor.
		STORM_CTOR TextWriter();

		// Write a single code point in UTF32.
		virtual void write(Nat codepoint);

		// Write a string.
		virtual void STORM_FN write(Str *str);

		// Write a line (appens an appropriate line ending to 'str').
		virtual void STORM_FN writeLine(Str *str);
	};



	/**
	 * Create a reader. Identifies the encoding automatically.
	 */
	TextReader *readText(Par<IStream> stream);

}
