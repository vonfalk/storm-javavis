#include "stdafx.h"
#include "ClientContext.h"
#include "Exception.h"
#include "SecureChannel.h"

namespace ssl {

	// TODO: Platform independent...

	ClientContext::ClientContext() {}

	Session *ClientContext::connect(IStream *input, OStream *output, Str *host) {
		SChannelContext *c = (SChannelContext *)data();
		return new (this) Session(input, output, new SChannelSession(c), host->c_str());
	}

	Session *ClientContext::connect(NetStream *socket, Str *host) {
		return connect(socket->input(), socket->output(), host);
	}

	Session *ClientContext::connect(NetStream *socket) {
		// TODO: Fixme!
		throw new (this) SSLError(L"Cannot do reverse lookups yet..."); // socket->remote()
	}

	SSLContext *ClientContext::createData() {
		return SChannelContext::createClient();
	}

}
