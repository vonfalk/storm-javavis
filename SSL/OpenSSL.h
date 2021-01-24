#pragma once

#ifdef POSIX
#include "Data.h"
#include "Core/Io/Buffer.h"
#include "Core/Io/Stream.h"

#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <openssl/crypto.h>

namespace ssl {

	/**
	 * OpenSSL context.
	 */
	class OpenSSLContext : public SSLContext {
	public:
		// Destroy.
		~OpenSSLContext();

		// Create a client context.
		static OpenSSLContext *createClient();

		// Create a server context.
		static OpenSSLContext *createServer();

		// Our OpenSSL context.
		SSL_CTX *context;

	private:
		// Create.
		OpenSSLContext(SSL_CTX *ctx);
	};

	/**
	 * OpenSSL session.
	 */
	class OpenSSLSession : public SSLSession {
	public:
		// Create from a context.
		OpenSSLSession(OpenSSLContext *ctx);

		// Destroy.
		~OpenSSLSession();

		// Session.
		OpenSSLContext *context;

		// Allocated BIO for the SSL connection.
		BIO *connection;

		// Create the session. Returns a pointer that needs to be kept alive by the GC.
		void *create(IStream *input, OStream *output, const char *host);
	};

}

#endif
