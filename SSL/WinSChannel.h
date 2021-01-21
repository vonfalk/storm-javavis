#pragma once

#ifdef WINDOWS
#include "OS/Sync.h"
#include "Data.h"
#include "Core/Io/Buffer.h"

namespace ssl {

	/**
	 * Various types we use on Windows.
	 */

	/**
	 * SSL credentials for SChannel.
	 */
	class SChannelContext : public SSLContext {
	public:
		virtual ~SChannelContext();

		// Acquired credentials.
		CredHandle credentials;

		// Create for a client.
		static SChannelContext *createClient();

	private:
		// Create.
		SChannelContext(CredHandle handle);
	};


	/**
	 * Data for a SSL session.
	 */
	class SChannelSession : public SSLSession {
	public:
		// Create.
		SChannelSession(SChannelContext *data, const String &host);

		virtual ~SChannelSession();

		// Owning data, so we don't free it too early.
		SChannelContext *data;

		// Session (called a context here). Initialized when we first update the context.
		CtxtHandle context;

		// Host.
		String host;

		// Maximum block size for encryption/decryption. Set when the session is established.
		Nat maxBlockSize;

		// Lock for the session.
		os::Lock lock;

		// Initialize a session. Reads data from "input" (empty at first), and writes to
		// "output". The function clears the part of "input" that was consumed, and may leave parts
		// of it there. "output" is to be sent to the remote peer.
		// Returns 0 if we're done, <0 if we need to send a message to the server, and >0 if we need
		// to get more data. Note: remaining size is just a guess.
		int initSession(Engine &e, Buffer &input, Buffer &output);
	};

}

#endif
