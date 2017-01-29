#pragma once
#include "Core/Io/Stream.h"

namespace storm {
	namespace server {
		STORM_PKG(core.lang.server);

		/**
		 * Fillable in-memory stream used for parsing of messages.
		 */
		class BufStream : public RIStream {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR BufStream();

			// Copy.
			BufStream(const BufStream &o);

			// Deep copy.
			void STORM_FN deepCopy(CloneEnv *env);

			// More data?
			virtual Bool STORM_FN more();

			// Read.
			using RIStream::read;
			virtual Buffer STORM_FN read(Buffer to);

			// Peek.
			using RIStream::peek;
			virtual Buffer STORM_FN peek(Buffer to);

			// Seek.
			virtual void STORM_FN seek(Word to);

			// Tell.
			virtual Word STORM_FN tell();

			// Current length.
			virtual Word STORM_FN length();

			// Get random access version.
			virtual RIStream *STORM_FN randomAccess();

			// Output.
			void STORM_FN toS(StrBuf *to) const;

			// Append more data. May discard any data which has already been read.
			void STORM_FN append(Buffer data);

			// Find the first occurrence of 'b' from 'tell()'. Returns the number of bytes to read
			// before reaching it, or the remaining number of bytes if none is found.
			Nat STORM_FN findByte(Byte b) const;

		private:
			// Data.
			Buffer data;

			// Position.
			Nat pos;

			// Re-allocate 'data' so that it can hold at least 'count' more bytes.
			void realloc(Nat count);
		};

	}
}
