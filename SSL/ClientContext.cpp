#include "stdafx.h"
#include "ClientContext.h"
#include "Exception.h"
#include "SecureChannel.h"
#include "OpenSSL.h"

namespace ssl {

	ClientContext::ClientContext() {}

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

	SSLContext *ClientContext::createData() {
		// TODO: We might want to allow using OpenSSL on Windows eventually.
#ifdef WINDOWS
		return SChannelContext::createClient();
#else
		return OpenSSLContext::createClient();
#endif
	}

}
