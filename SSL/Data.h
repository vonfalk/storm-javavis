#pragma once
#include "OS/Sync.h"
#include "Core/Str.h"
#include "Core/Io/Stream.h"

namespace ssl {

	/**
	 * Generic data utilities.
	 */

	class SSLSession;
	class WinSSLCert;
	class OpenSSLCert;

	/**
	 * Generic refcount class.
	 */
	class RefCount {
	public:
		// Create, initializes refs to 1.
		RefCount() : refs(1) {}

		// Destroy.
		virtual ~RefCount() {}

		// Add a reference.
		void ref() {
			atomicIncrement(refs);
		}

		// Release a reference.
		void unref() {
			if (atomicDecrement(refs) == 0) {
				delete this;
			}
		}

	private:
		// References.
		size_t refs;
	};

	/**
	 * Data for an SSL context. Internal to the Context, but convenient to have outside the class
	 * declaration.
	 */
	class SSLContext : public RefCount {
	public:
		// Create a session for this context.
		virtual SSLSession *createSession() = 0;
	};

	/**
	 * Data for an individual session.
	 */
	class SSLSession : public RefCount {
	public:
		// Lock for the session.
		os::Lock lock;

		// Connect this session. Returns any additional data needed to be kept alive by the GC.
		virtual void *connect(IStream *input, OStream *output, Str *host) = 0;

		// More data available?
		virtual Bool more(void *gcData) = 0;

		// Read data.
		virtual void read(Buffer &to, void *gcData) = 0;

		// Peek data.
		virtual void peek(Buffer &to, void *gcData) = 0;

		// Write data.
		virtual void write(const Buffer &from, Nat start, void *gcData) = 0;

		// Flush the stream.
		virtual void flush(void *gcData) = 0;

		// Shut down the session.
		virtual void shutdown(void *gcData) = 0;

		// Close the underlying streams.
		virtual void close(void *gcData) = 0;
	};


	/**
	 * Some representation of a certificate.
	 *
	 * The certificate inside is expected to be read-only after creation.
	 */
	class SSLCert : public RefCount {
	public:
		// Get the windows SSL implementation (might convert it, might throw).
		virtual WinSSLCert *windows() = 0;

		// Get the OpenSSL implementation version (might convert it, might throw).
		virtual OpenSSLCert *openSSL() = 0;

		// Extract some sensible string data.
		virtual void output(StrBuf *to) = 0;
	};

}
