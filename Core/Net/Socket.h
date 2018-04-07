#pragma once
#include "Net.h"
#include "Address.h"
#include "Core/Timing.h"
#include "OS/Handle.h"

namespace storm {
	STORM_PKG(core.net);

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
		Socket(os::Handle handle);

		// Destroy.
		virtual ~Socket();

		// Close the socket. Calls 'close' on both the input and output streams.
		virtual void STORM_FN close();

		// To string.
		void STORM_FN toS(StrBuf *to) const;

		// Get input timeout.
		Duration STORM_FN inputTimeout() const;

		// Set input timeout.
		void STORM_ASSIGN inputTimeout(Duration v);

		// Get output timeout.
		Duration STORM_FN outputTimeout() const;

		// Set output timeout.
		void STORM_ASSIGN outputTimeout(Duration v);

	protected:
		// The handle for this socket.
		UNKNOWN(PTR_NOGC) os::Handle handle;
	};

}
