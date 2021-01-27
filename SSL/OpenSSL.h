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

		// Create a session.
		virtual SSLSession *createSession();

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
		virtual ~OpenSSLSession();

		// Implementation of the generic interface.
		virtual void *connect(IStream *input, OStream *output, Str *host);
		virtual Bool more(void *gcData);
		virtual void read(Buffer &to, void *gcData);
		virtual void peek(Buffer &to, void *gcData);
		virtual void write(const Buffer &from, Nat start, void *gcData);
		virtual void flush(void *gcData);
		virtual void shutdown(void *gcData);
		virtual void close(void *gcData);

	private:
		// Session.
		OpenSSLContext *context;

		// Allocated BIO for the SSL connection.
		BIO *connection;
	};

}

#endif
