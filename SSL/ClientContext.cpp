#include "stdafx.h"
#include "ClientContext.h"
#include "Exception.h"
#include "SecureChannel.h"
#include "OpenSSL.h"

namespace ssl {

	// TODO: Platform independent...

	ClientContext::ClientContext() {}

	Session *ClientContext::connect(IStream *input, OStream *output, Str *host) {
#ifdef WINDOWS
		SChannelContext *c = (SChannelContext *)data();
		return new (this) Session(input, output, new SChannelSession(c), host->c_str());
#else
		data();
		return null;
#endif
	}

	Session *ClientContext::connect(NetStream *socket, Str *host) {
		return connect(socket->input(), socket->output(), host);
	}

	Session *ClientContext::connect(NetStream *socket) {
		// TODO: Fixme!
		throw new (this) SSLError(S("Cannot do reverse lookups yet...")); // socket->remote()
	}

	SSLContext *ClientContext::createData() {
#ifdef WINDOWS
		return SChannelContext::createClient();
#else
		return OpenSSLContext::createClient();
#endif
	}

}
