#pragma once
#include "Stream.h"
#include "Core/EnginePtr.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * Streams wrapping a OS handle. These are used in various places in the implementation, but are
	 * not intended for direct manipulation by the user.
	 */


	/**
	 * Opaque wrapper of a system dependent OS handle to avoid mistakingly casting the handle to an
	 * erroneous type. This has to be pointer-sized.
	 *
	 * This implementation assumes that all OS handles are set to asynchronious mode.
	 */
#ifdef WINDOWS
	class OSHandle {
	public:
		inline OSHandle() : z(INVALID_HANDLE_VALUE) {}
		inline OSHandle(HANDLE v) : z(v) {}
		inline HANDLE v() { return z; }
		inline operator bool () { return z != INVALID_HANDLE_VALUE; }
	private:
		HANDLE z;
	};
#else
	class OSHandle {
	public:
		inline OSHandle() : z(-1) {}
		inline OSHandle(int v) : z(v) {}
		inline int v() { return (int)this->z; }
		inline operator bool () { return v() >= 0; }
	private:
		ptrdiff_t z;
	};
#endif

	/**
	 * OS input stream.
	 */
	class OSIStream : public IStream {
		STORM_CLASS;
	public:
		// Create.
		OSIStream(OSHandle handle);

		// Copy.
		OSIStream(const OSIStream &o);

		// Destroy.
		virtual ~OSIStream();

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *e);

		// Are we at the end of the stream?
		virtual Bool STORM_FN more();

		// Read a buffer from the stream. Returns the number of bytes read.
		using IStream::read;
		virtual Buffer STORM_FN read(Buffer to, Nat start);

		// Peek data.
		using IStream::peek;
		virtual Buffer STORM_FN peek(Buffer to, Nat start);

		// Close this stream.
		virtual void STORM_FN close();

	protected:
		// Our handle.
		UNKNOWN(PTR_NOGC) OSHandle handle;

	private:
		// Lookahead data (if any). Used when doing peek() operations.
		GcArray<byte> *lookahead;

		// How much of 'lookahead' have we consumed so far?
		Nat lookaheadStart;

		// Have wee seen the end of this stream?
		Bool atEof;

		// Try to fill the lookahead buffer with 'bytes' bytes. Returns the number of available
		// bytes in the lookahead buffer.
		Nat doLookahead(Nat bytes);

		// Get the number of bytes available in the lookahead (which are filled).
		Nat lookaheadAvail();

		// Ensure there is at least 'n' bytes in the lookahead buffer (which may or may not be filled).
		void ensureLookahead(Nat n);
	};

	/**
	 * Random access OS input stream.
	 */
	class OSRIStream : public RIStream {
		STORM_CLASS;
	public:
		// Create.
		OSRIStream(OSHandle handle);

		// Copy.
		OSRIStream(const OSRIStream &o);

		// Destroy.
		virtual ~OSRIStream();

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *e);

		// Are we at the end of the stream?
		virtual Bool STORM_FN more();

		// Read a buffer from the stream. Returns the number of bytes read.
		using IStream::read;
		virtual Buffer STORM_FN read(Buffer to, Nat start);

		// Peek data.
		using IStream::peek;
		virtual Buffer STORM_FN peek(Buffer to, Nat start);

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
		UNKNOWN(PTR_NOGC) OSHandle handle;
	};

	/**
	 * OS output stream.
	 */
	class OSOStream : public OStream {
		STORM_CLASS;
	public:
		// Create.
		OSOStream(OSHandle h);

		// Copy.
		OSOStream(const OSOStream &o);

		// Destroy.
		~OSOStream();

		// Write data.
		using OStream::write;
		virtual void STORM_FN write(Buffer buf, Nat start);

		// Close.
		virtual void STORM_FN close();

	protected:
		// Our handle.
		UNKNOWN(PTR_NOGC) OSHandle handle;
	};

}
