#pragma once
#include "Core/Object.h"

namespace storm {
	STORM_PKG(storm.io);

	/**
	 * Input and Output streams for Storm. These are abstract base classes
	 * and concrete implementations are provided for at least file system IO
	 * and standard input/output.
	 *
	 * When cloning streams, you get a new read-pointer to the underlying source.
	 * TODO: In some cases (such as networks) this does not makes sense...
	 */

	class RIStream;

	/**
	 * Input stream.
	 */
	class IStream : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR IStream();

		// Copy.
		STORM_CTOR IStream(IStream *o);

		// Are we at the end of the stream?
		virtual Bool STORM_FN more();

		// Read a buffer from the stream. Returns the number of bytes read.
		// virtual Nat STORM_FN read(Buffer &to);

		// Peek data.
		// virtual Nat STORM_FN peek(Buffer &to);

		// Get a random access IStream. May return the same stream!
		virtual RIStream *STORM_FN randomAccess();
	};

	/**
	 * Random access input stream.
	 */
	class RIStream : public IStream {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR RIStream();
		STORM_CTOR RIStream(RIStream *o);

		// This implementation returns the same object.
		virtual RIStream *STORM_FN randomAccess();

		/**
		 * Extended interface.
		 */

		// Seek relative the start.
		virtual void STORM_FN seek(Word to);

		// Get current position.
		virtual Word STORM_FN tell();

		// Get length.
		virtual Word STORM_FN length();
	};


	/**
	 * Output stream.
	 */
	class OStream : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR OStream();

		// Copy.
		STORM_CTOR OStream(OStream *o);

		// Write some data.
		// virtual void STORM_FN write(const Buffer &buf);
	};

}
