#pragma once
#include "Context.h"
#include "Session.h"
#include "Certificate.h"
#include "Core/Net/NetStream.h"

namespace ssl {

	/**
	 * A context used to represent a server context.
	 *
	 * A server generally needs to provide a certificate of some sort to assert its identity.
	 */
	class ServerContext : public Context {
		STORM_CLASS;
	public:
		// Create a server context with a pre-defined certificate and key.
		STORM_CTOR ServerContext(CertificateKey *key);

		// Create a new session. Returns once the handshake (initiated by the remote end) is complete.
		Session *STORM_FN connect(IStream *input, OStream *output);

		// Connect using a TCP socket for simplicity.
		Session *STORM_FN connect(NetStream *stream);

	protected:
		// Create data.
		virtual SSLContext *createData();

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Certificate and key.
		CertificateKey *key;
	};

}
