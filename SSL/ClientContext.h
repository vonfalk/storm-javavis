#pragma once
#include "Context.h"
#include "Session.h"
#include "Certificate.h"
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
		static ClientContext *STORM_FN systemDefault(EnginePtr e);

		// Create a client context that will only trust a particular certificate. By default, we
		// don't verify the hostname of the certificate.
		static ClientContext *STORM_FN pinnedTo(Certificate *certificate);

		// Verify the hostname?
		Bool STORM_FN verifyHostname() { return checkHostname; }
		void STORM_ASSIGN verifyHostname(Bool v);

		// Only use strong ciphers. Disables cipher suites with known weaknesses, but compatibility may suffer.
		Bool STORM_FN strongCiphers() { return onlyStrong; }
		void STORM_ASSIGN strongCiphers(Bool b);

		// Get pinned certificate, if any.
		MAYBE(Certificate *) STORM_FN pinnedCertificate() { return pinned; }

		// Create an endpoint.
		Session *STORM_FN connect(IStream *input, OStream *output, Str *host);

		// Connect using a TCP socket, but don't use the hostname from the remote peer.
		Session *STORM_FN connect(NetStream *stream, Str *host);

		// Connect using a TCP socket, trying to figure out the hostname from the remote peer.
		Session *STORM_FN connect(NetStream *stream);

	protected:
		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Create data.
		virtual SSLContext *createData();

	private:
		// Create.
		ClientContext();

		// Certificate to trust, if any.
		Certificate *pinned;

		// Verify hostname?
		Bool checkHostname;

		// Only strong ciphers.
		Bool onlyStrong;
	};

}
