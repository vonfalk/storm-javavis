#pragma once
#include "Context.h"
#include "Session.h"
#include "Core/Net/NetStream.h"

namespace ssl {

	/**
	 * A context used to represent a client context.
	 *
	 * A client generally does not need to assert its identity, but desires to know the identity of
	 * the remote peer of the connection.
	 *
	 * TODO: Some of the 'connect' functions would be good to move to Context. We'll see exactly
	 * which ones when the server is done.
	 */
	class ClientContext : public Context {
		STORM_CLASS;
	public:
		// Create a default client context. It will by default use the system's certificate store
		// and the system's default trusted ciphers.
		STORM_CTOR ClientContext();

		// Create an endpoint.
		Session *STORM_FN connect(IStream *input, OStream *output, Str *host);

		// Connect using a TCP socket, but don't use the hostname from the remote peer.
		Session *STORM_FN connect(NetStream *stream, Str *host);

		// Connect using a TCP socket, trying to figure out the hostname from the remote peer.
		Session *STORM_FN connect(NetStream *stream);

	protected:
		// Create data.
		virtual SSLContext *createData();
	};

}
