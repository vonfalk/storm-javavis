#pragma once
#include "Stream.h"
#include "PeekStream.h"
#include "OS/Handle.h"
#include "Core/EnginePtr.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * Streams wrapping a OS handle. These are used in various places in the implementation, but are
	 * not intended for direct manipulation by the user.
	 */


	/**
	 * Handle input stream.
	 */
	class HandleIStream : public PeekIStream {
		STORM_CLASS;
	public:
		// Create.
		HandleIStream(os::Handle handle);
		HandleIStream(os::Handle handle, os::Thread attachedTo);

		// Copy.
		HandleIStream(const HandleIStream &o);

		// Destroy.
		virtual ~HandleIStream();

		// More data?
		virtual Bool STORM_FN more();

		// Close this stream.
		virtual void STORM_FN close();

	protected:
		// Our handle.
		UNKNOWN(PTR_NOGC) os::Handle handle;

		// Is our handle added to a thread?
		UNKNOWN(PTR_NOGC) os::Thread attachedTo;

		// Do read operations.
		virtual Nat doRead(byte *to, Nat count);
	};

	/**
	 * Random access Handle input stream.
	 */
	class HandleRIStream : public RIStream {
		STORM_CLASS;
	public:
		// Create.
		HandleRIStream(os::Handle handle);
		HandleRIStream(os::Handle handle, os::Thread attachedTo);

		// Copy.
		HandleRIStream(const HandleRIStream &o);

		// Destroy.
		virtual ~HandleRIStream();

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *e);

		// Are we at the end of the stream?
		virtual Bool STORM_FN more();

		// Read a buffer from the stream. Returns the number of bytes read.
		using IStream::read;
		virtual Buffer STORM_FN read(Buffer to);

		// Peek data.
		using IStream::peek;
		virtual Buffer STORM_FN peek(Buffer to);

		// Get a random access IStream. May return the same stream!
		virtual RIStream *STORM_FN randomAccess();

		// Close this stream.
		virtual void STORM_FN close();

		// Seek relative the start.
		virtual void STORM_FN seek(Word to);

		// Get current position.
		virtual Word STORM_FN tell();

		// Get length.
		virtual Word STORM_FN length();

	protected:
		// Our handle.
		UNKNOWN(PTR_NOGC) os::Handle handle;

		// Is our handle added to a thread?
		UNKNOWN(PTR_NOGC) os::Thread attachedTo;
	};

	/**
	 * Handle output stream.
	 */
	class HandleOStream : public OStream {
		STORM_CLASS;
	public:
		// Create.
		HandleOStream(os::Handle h);
		HandleOStream(os::Handle h, os::Thread attachedTo);

		// Copy.
		HandleOStream(const HandleOStream &o);

		// Destroy.
		virtual ~HandleOStream();

		// Write data.
		using OStream::write;
		virtual void STORM_FN write(Buffer buf, Nat start);

		// Close.
		virtual void STORM_FN close();

	protected:
		// Our handle.
		UNKNOWN(PTR_NOGC) os::Handle handle;

		// Is our handle added to a thread?
		UNKNOWN(PTR_NOGC) os::Thread attachedTo;
	};

}
