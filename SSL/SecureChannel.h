#pragma once

#ifdef WINDOWS
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
		SChannelSession(SChannelContext *data);

		virtual ~SChannelSession();

		// Owning data, so we don't free it too early.
		SChannelContext *data;

		// Session (called a context here). Initialized when we first update the context.
		CtxtHandle context;

		// Host.
		String host;

		// Maximum message size for encryption/decryption. Set when the session is established.
		Nat maxMessageSize;

		// Block size for the cipher. Optimal encoding if messages are a size modulo this size (seems to be 16 often)
		Nat blockSize;

		// Initialize a session. Reads data from "input" (empty at first), and writes to
		// "output". The function clears the part of "input" that was consumed, and may leave parts
		// of it there. "output" is to be sent to the remote peer.
		// Returns 0 if we're done, <0 if we need to send a message to the server, and >0 if we need
		// to get more data. Note: remaining size is just a guess.
		int initSession(Engine &e, Buffer &input, Buffer &output, const wchar *host);

		// Encrypt a message using this session.
		void encrypt(Engine &e, const Buffer &input, Nat inputOffset, Buffer &output);

		// Markers from 'decrypt'.
		struct Markers {
			// Start and end of decrypted plaintext.
			Nat plaintextStart;
			Nat plaintextEnd;

			// Start of any remaining data that was not decrypted.
			Nat remainingStart;

			// Is this the end of the connection?
			Bool shutdown;
		};

		// Decrypt a message using this session. Decryption is performed in-place. Returns a set of
		// markers indicating where in the data different parts can be found.
		Markers decrypt(Engine &e, Buffer &data, Nat offset);

		// Shutdown the channel. Returns a message to send to the remote peer.
		Buffer shutdown(Engine &e);

	private:
		// Initialize sizes. Called when the context is established.
		void initSizes();

		// Buffer sizes for headers and trailers.
		Nat headerSize;
		Nat trailerSize;
	};

}

#endif
