#include "stdafx.h"
#include "ServerContext.h"
#include "SecureChannel.h"

namespace ssl {

	ServerContext::ServerContext() {}

	SSLContext *ServerContext::createData() {
#ifdef WINDOWS
		return SChannelContext::createServer();
#else
		return OpenSSLContext::createServer();
#endif
	}

}
