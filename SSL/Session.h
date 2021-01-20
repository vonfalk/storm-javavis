#pragma once
#include "Core/Io/Stream.h"
#include "Data.h"

namespace ssl {

	/**
	 * An SSL endpoint is a bidirectional stream (like a socket) that is encrypted using SSL of some
	 * kind (typically TLS nowadays).
	 *
	 * A typical usage of this class is to wrap a regular Socket to communicate over SSL. As such, a
	 * special constructor is available for this particular use case. It is, however, possible to
	 * use it on any pair of streams.
	 *
	 * Use ssl:Context to create endpoints.
	 */
	class Session : public Object {
		STORM_CLASS;
	public:
		// Create from other parts of the system. Will initiate a SSL context.
		Session(IStream *input, OStream *output, SSLSession *session);

		// Copy.
		Session(const Session &o);

		// Destroy.
		virtual ~Session();

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		// Close the connection.
		virtual void close();

		// Get input stream.
		// IStream *STORM_FN input() const;

		// Get output stream.
		// OStream *STORM_FN output() const;

	private:
		// Input stream.
		IStream *src;

		// Output stream.
		OStream *dst;

		// SSL context.
		SSLSession *data;
	};

}
