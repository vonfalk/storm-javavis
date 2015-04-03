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


	namespace utf16 {
		/**
		 * Utf16 helpers. Also used in Text.cpp.
		 */

		static inline bool leading(nat16 ch) {
			// return (ch >= 0xD800) && (ch < 0xDC00);
			return (ch & 0xFC00) == 0xD800;
		}

		static inline bool trailing(nat16 ch) {
			// return (ch >= 0xDC00) && (ch < 0xE000);
			return (ch & 0xFC00) == 0xDC00;
		}

		static inline nat assemble(nat16 lead, nat16 trail) {
			nat r = nat(lead & 0x3FF) << nat(10);
			r |= nat(trail & 0x3FF);
			r += 0x10000;
			return r;
		}
	}

}
