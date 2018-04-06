#pragma once
#include "Net.h"
#include "Address.h"
#include "Core/Io/HandleStream.h"
#include "OS/Handle.h"

namespace storm {
	STORM_PKG(core.net);

	class SocketIStream;
	class SocketOStream;

	/**
	 * A TCP socket.
	 *
	 * The socket contain two streams: one for reading data and another for writing data. These
	 * streams may be used simultaneously. As for regular streams, it is not a good idea to share
	 * these streams between different threads, even if it is possible.
	 *
	 * Sockets are created either by calling the 'connect' function, or by a Listener object.
	 */
	class Socket : public Object {
		STORM_CLASS;
	public:
		// Create. Assumes the socket in 'handle' is set up for asynchronious operation.
		Socket(os::Handle handle, os::Thread attachTo);

		// Destroy.
		virtual ~Socket();

		// Close the socket. Calls 'close' on both the input and output streams.
		virtual void STORM_FN close();

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Get the input stream.
		IStream *STORM_FN input() const;

		// Get the output stream.
		OStream *STORM_FN output() const;

	private:
		friend class SocketIStream;
		friend class SocketOStream;

		// The handle for this socket.
		UNKNOWN(PTR_NOGC) os::Handle handle;

		// Closed ends of the socket.
		Nat closed;

		enum {
			closeRead = 0x1,
			closeWrite = 0x2
		};

		// Close one end of the socket.
		void closeEnd(Nat which);

		// Input and output streams.
		SocketIStream *i;
		SocketOStream *o;
	};

	// Create a socket that is connected to a specific address.
	MAYBE(Socket *) STORM_FN connect(Address *to);


	/**
	 * Input stream for the socket.
	 */
	class SocketIStream : public HandleIStream {
	public:
		// Not exposed to Storm. Created by the Socket.
		SocketIStream(Socket *owner, os::Thread attachedTo);

		// Destroy.
		virtual ~SocketIStream();

		// Close this stream.
		virtual void STORM_FN close();

	private:
		// Owner.
		Socket *owner;
	};


	/**
	 * Output stream for the socket.
	 */
	class SocketOStream : public HandleOStream {
		STORM_CLASS;
	public:
		// Not exposed to Storm. Created by the socket.
		SocketOStream(Socket *owner, os::Thread attachedTo);

		// Destroy.
		virtual ~SocketOStream();

		// Close.
		virtual void STORM_FN close();

	private:
		// Owner.
		Socket *owner;
	};

}
