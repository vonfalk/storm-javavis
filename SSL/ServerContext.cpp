#include "stdafx.h"
#include "ServerContext.h"
#include "SecureChannel.h"

namespace ssl {

	ServerContext::ServerContext(CertificateKey *key) : key(key) {}

	SSLContext *ServerContext::createData() {
#ifdef WINDOWS
		return SChannelContext::createServer(this, key);
#else
		return OpenSSLContext::createServer(this, key);
#endif
	}

	Session *ServerContext::connect(IStream *input, OStream *output) {
		return new (this) Session(input, output, data()->createSession(), null);
	}

	Session *ServerContext::connect(NetStream *socket) {
		return connect(socket->input(), socket->output());
	}

	void ServerContext::toS(StrBuf *to) const {
		*to << S("Server context:\n");
		*to << S("Certificate: ") << key;
	}

}
