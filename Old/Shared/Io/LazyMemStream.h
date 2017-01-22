#pragma once
#include "Stream.h"

namespace storm {

	/**
	 * A stream that lazily loads data from another stream. This stream is used to make other streams
	 * seekable. Note that 'length' will be updated throughout the lifetime.
	 */
	class LazyIMemStream : public RIStream {
		STORM_CLASS;
	public:
		// Copy.
		STORM_CTOR LazyIMemStream(Par<LazyIMemStream> o);

		// Destroy.
		~LazyIMemStream();

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

		// Size
		virtual Word STORM_FN length();

	private:
		friend LazyIMemStream *lazyIMemStream(Par<IStream> src);
		LazyIMemStream();

		// Source stream.
		Auto<IStream> src;

		// Data.
		byte *buffer;

		// Length of buffer.
		nat len;

		// Position.
		nat pos;

		// How much to grow?
		static const nat grow = 1024;

		// Ensure we have 'n' bytes in the buffer (counted from pos).
		bool ensure(nat n);
	};

	// Create from a stream.
	LazyIMemStream *STORM_FN lazyIMemStream(Par<IStream> src);

}
