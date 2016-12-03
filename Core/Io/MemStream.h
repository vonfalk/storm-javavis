#pragma once
#include "Buffer.h"
#include "Stream.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * Simple memory stream.
	 */
	class IMemStream : public RIStream {
		STORM_CLASS;
	public:
		// Create from a buffer.
		STORM_CTOR IMemStream(Buffer b);

		// Copy.
		STORM_CTOR IMemStream(IMemStream *o);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// More?
		virtual Bool STORM_FN more();

		// Read.
		using RIStream::read;
		virtual Buffer STORM_FN read(Buffer to, Nat start);

		// Peek.
		using RIStream::peek;
		virtual Buffer STORM_FN peek(Buffer to, Nat start);

		// Seek.
		virtual void STORM_FN seek(Word to);

		// Peek.
		virtual Word STORM_FN tell();

		// Length.
		virtual Word STORM_FN length();

		// Random access.
		virtual RIStream *STORM_FN randomAccess();

	private:
		// Data.
		Buffer data;

		// Position.
		nat pos;
	};

}
