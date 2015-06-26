#pragma once
#include "Stream.h"

namespace storm {

	/**
	 * Simple memory stream.
	 */
	class IMemStream : public RIStream {
		STORM_CLASS;
	public:
		// Create from a buffer.
		STORM_CTOR IMemStream(const Buffer &b);

		// Copy.
		STORM_CTOR IMemStream(Par<IMemStream> o);

		// Deep copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> env);

		// More?
		virtual Bool STORM_FN more();

		// Read
		virtual Nat STORM_FN read(Buffer &to);

		// Peek.
		virtual Nat STORM_FN peek(Buffer &to);

		// Seek.
		virtual void STORM_FN seek(Word to);

		// Tell.
		virtual Word STORM_FN tell();

		// Length.
		virtual Word STORM_FN length();

	private:
		// Data
		Buffer data;

		// Position.
		nat pos;
	};


	/**
	 * Output memory stream.
	 */
	class OMemStream : public OStream {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR OMemStream();

		// Copy.
		STORM_CTOR OMemStream(Par<OMemStream> o);

		// Dtor.
		~OMemStream();

		// Deep copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> env);

		// Write.
		virtual void STORM_FN write(const Buffer &buf);

		// Get a buffer with the contents of this stream.
		virtual Buffer STORM_FN buffer();

	private:
		// Output buffer.
		byte *data;

		// Output buffer size.
		nat capacity;

		// Current position (filled to).
		nat pos;

		// Ensure capacity.
		void ensure(nat size);
	};

}
