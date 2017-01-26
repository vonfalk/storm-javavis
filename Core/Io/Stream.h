#pragma once
#include "Core/Object.h"
#include "Buffer.h"

namespace storm {
	STORM_PKG(core.io);

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

		// Are we at the end of the stream?
		virtual Bool STORM_FN more();

		// Attempts to fill the remainder of the buffer by reading data from the stream. Starts
		// filling from the 'filled' parameter of the buffer. Returns the resulting buffer (which
		// might point to the original data), with a modified 'filled' parameter. The resulting
		// 'filled' parameter is never smaller than it was in the buffer passed to the stream.
		Buffer STORM_FN read(Nat maxBytes);
		virtual Buffer STORM_FN read(Buffer to);

		// Peek data. Semantics are the same as 'read', but bytes are not consumed. This means that
		// the next 'read' or 'peek' operation will see the same bytes the 'peek' operation saw.
		Buffer STORM_FN peek(Nat maxBytes);
		virtual Buffer STORM_FN peek(Buffer to);

		// Reads data until the buffer is filled or at the end of stream. Compared to 'read', after
		// calling 'readAll' you know that either your buffer is full or the end of the stream was
		// reached.
		Buffer STORM_FN readAll(Nat bytes);
		Buffer STORM_FN readAll(Buffer to);

		// Get a random access IStream. May return the same stream!
		virtual RIStream *STORM_FN randomAccess();

		// Close this stream.
		virtual void STORM_FN close();
	};

	/**
	 * Random access input stream.
	 */
	class RIStream : public IStream {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR RIStream();

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

		// Write all data from the beginning of the buffer (or start, if specified) until the
		// buffer's 'filled' marker.
		void STORM_FN write(Buffer buf);
		virtual void STORM_FN write(Buffer buf, Nat start);

		// Flush this stream.
		virtual void STORM_FN flush();

		// Close.
		virtual void STORM_FN close();
	};

}
