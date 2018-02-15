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
	 * TODO: Add TextInfo to a TextReader as well!
	 */

	/**
	 * Information about encoded text.
	 */
	class TextInfo {
		STORM_VALUE;
	public:
		// Default config.
		STORM_CTOR TextInfo();

		// Use windows-style linefeeds.
		Bool useCrLf;

		// Output a BOM.
		Bool useBom;
	};

	// System default TextInfo.
	TextInfo STORM_FN sysTextInfo();

	/**
	 * Base interface for reading text. Caches one character. When implementing your own version,
	 * override 'readChar' to read a code point in UTF32. Manages line endings so that the 'read'
	 * functions will always only see '\n' as line endings. 'readRaw' and 'peek' may observe a '\r'
	 * sometimes.
	 */
	class TextInput : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR TextInput();

		// Read a single character from the stream. Returns 0 on failure.
		Char STORM_FN read();

		// Read a single character from the stream without line-ending conversion. Returns 0 on failure.
		Char STORM_FN readRaw();

		// Peek a single character. Returns 0 on failure.
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

		// Is 'next' valid?
		Bool hasNext;

		// First character?
		Bool first;

		// At end of file.
		Bool eof;

		// Helper for reading which removes any BOM.
		Char doRead();
	};


	// Create a reader. Identifies the encoding automatically.
	TextInput *STORM_FN readText(IStream *stream);

	// Read a string.
	TextInput *STORM_FN readStr(Str *from);

	// Read all text from a file.
	Str *STORM_FN readAllText(Url *file);


	/**
	 * Base interface for writing text. Buffers entire lines. When implementing your own version,
	 * override 'writeChar' and 'flush' to write a code point in UTF32.
	 */
	class TextOutput : public Object {
		STORM_CLASS;
	public:
		// Create. Output regular unix line endings (TODO: Should this be OS-dependent?)
		STORM_CTOR TextOutput();

		// Create. Specify line endings.
		STORM_CTOR TextOutput(TextInfo info);

		// Automatic flush on newline? (on by default)
		Bool autoFlush;

		// Write a string.
		void STORM_FN write(Char c);
		void STORM_FN write(Str *s);

		// Write a string, add any line ending.
		void STORM_FN writeLine(Str *s);

		// Write a new-line character.
		void STORM_FN writeLine();

		// Flush all buffered output to the underlying stream.
		virtual void STORM_FN flush();

	protected:
		// Override in derived writers. Write one character.
		virtual void STORM_FN writeChar(Char ch);

	private:
		// Text config.
		TextInfo config;

		// Write a bom if needed.
		void writeBom();
	};

}
