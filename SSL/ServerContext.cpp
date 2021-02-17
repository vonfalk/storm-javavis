#include "stdafx.h"
#include "ServerContext.h"
#include "SecureChannel.h"

namespace ssl {

	ServerContext::ServerContext() {
		data();
	}

	SSLContext *ServerContext::createData() {
#ifdef WINDOWS
		return SChannelContext::createServer();
#else
		return OpenSSLContext::createServer();
#endif
	}

}
