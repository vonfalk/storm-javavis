#include "stdafx.h"
#include "ClientContext.h"
#include "Exception.h"
#include "SecureChannel.h"
#include "OpenSSL.h"

namespace ssl {

	ClientContext *ClientContext::systemDefault(EnginePtr e) {
		return new (e.v) ClientContext();
	}

	ClientContext *ClientContext::pinnedTo(Certificate *to) {
		ClientContext *c = new (to) ClientContext();
		c->pinned = to;
		c->checkHostname = false;
		return c;
	}

	ClientContext::ClientContext() : checkHostname(true) {}

	void ClientContext::verifyHostname(Bool v) {
		checkHostname = v;
		invalidate();
	}

	Session *ClientContext::connect(IStream *input, OStream *output, Str *host) {
		return new (this) Session(input, output, data()->createSession(), host);
	}

	Session *ClientContext::connect(NetStream *socket, Str *host) {
		return connect(socket->input(), socket->output(), host);
	}

	Session *ClientContext::connect(NetStream *socket) {
		// TODO: Fixme!
		throw new (this) SSLError(S("Cannot do reverse lookups yet...")); // socket->remote()
	}

	void ClientContext::toS(StrBuf *to) const {
		*to << S("Client context:\n");
		if (pinned)
			*to << S("Pinned to: ") << pinned << S("\n");
		*to << S("Verify hostname: ") << (checkHostname ? S("yes\n") : S("no\n"));
		*to << S("Only strong ciphers: ") << (strongCiphers() ? S("yes") : S("no"));
	}

	SSLContext *ClientContext::createData() {
		// TODO: We might want to allow using OpenSSL on Windows eventually.
#ifdef WINDOWS
		return SChannelContext::createClient(this);
#else
		return OpenSSLContext::createClient(this);
#endif
	}

}
