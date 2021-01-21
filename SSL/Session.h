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
	 *
	 * This class will not be "properly" cloned when crossing thread boundaries. We protect all
	 * relevant data with locks though, and most data is constant after construction.
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
		virtual void STORM_FN close();

		// Get input stream.
		IStream *STORM_FN input();

		// Get output stream.
		OStream *STORM_FN output();

	private:
		// Input stream.
		IStream *src;

		// Output stream.
		OStream *dst;

		// SSL context.
		SSLSession *data;

		// Input and output buffers used when encrypting/decrypting data.
		Buffer inBuffer;
		Buffer outBuffer;
	};


	/**
	 * Input stream for a session.
	 *
	 * This class is not really cloned, we store all data inside Session, and let that class handle
	 * all state. There, we protect the data with locks as needed.
	 */
	class SessionIStream : public IStream {
		STORM_CLASS;
	public:
		// Created by the Session.
		SessionIStream(Session *owner);

		// Close it.
		virtual void STORM_FN close();

		// More data?
		virtual Bool STORM_FN more();

		// Read data.
		virtual Buffer STORM_FN read(Buffer to);

		// Peek data.
		virtual Buffer STORM_FN peek(Buffer to);

	private:
		// Session.
		Session *owner;
	};


	/**
	 * Output stream for a session.
	 *
	 * This class is not really cloned, we store all data inside Session, and let that class handle
	 * all state. There, we protect the data with locks as needed.
	 */
	class SessionOStream : public OStream {
		STORM_CLASS;
	public:
		// Created by the Session.
		SessionOStream(Session *owner);

		// Close it.
		virtual void STORM_FN close();

		// Write data.
		virtual void STORM_FN write(Buffer buf, Nat start);

		// Flush the stream.
		virtual void STORM_FN flush();

	private:
		// Session.
		Session *owner;
	};


}
