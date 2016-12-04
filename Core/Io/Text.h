#pragma once
#include "Core/Object.h"
#include "Core/Char.h"
#include "Url.h"
#include "Stream.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * Text IO. Supports UTF8 and UTF16, little and big endian. Use 'readText' to detect which
	 * encoding is used and create the correct reader for that encoding.
	 *
	 * Line endings are always converted into a single \n (ie. Unix line endings). If the original
	 * line endings are desired, use 'readAllRaw' on a TextReader. Supports \r\n (Windows), \n
	 * (Unix) and \r (Macintosh).
	 *
	 * TODO: This should also be extensible.
	 * TODO: Better handling of line endings.
	 * TODO: Find a good way of duplicating an input format to an output format.
	 */


	/**
	 * Base interface for reading text. Caches one character. When implementing your own version,
	 * override 'readChar' to read a code point in UTF32.
	 */
	class TextReader : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR TextReader();

		// Read a single character from the stream. Returns 0 on failure.
		Char STORM_FN read();

		// Peek a single character.
		Char STORM_FN peek();

		// Read an entire line from the file (does not care about line endings).
		Str *STORM_FN readLine();

		// Read the entire file.
		Str *STORM_FN readAll();

		// Read the entire file raw (still ignores any BOM).
		Str *STORM_FN readAllRaw();

		// More data in the file?
		Bool STORM_FN more();

	protected:
		// Override in derived readers, read one character.
		virtual Char STORM_FN readChar();

	private:
		// Cached code point. 0 if at end of stream.
		Char next;

		// First code point?
		Bool first;
	};


	// Create a reader. Identifies the encoding automatically.
	TextReader *STORM_FN readText(IStream *stream);

	// Read a string.
	TextReader *STORM_FN readStr(Str *from);

	// Read all text from a file.
	Str *STORM_FN readAllText(Url *file);
}
